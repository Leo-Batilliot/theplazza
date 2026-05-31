#include "reception/Reception.hpp"

#include "common/Errors.hpp"
#include "reception/SignalHandler.hpp"

#include <chrono>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <variant>

namespace plazza::reception {

namespace {

std::string pizzaName(pizza::PizzaType type)
{
    switch (type) {
        case pizza::PizzaType::Regina:    return "regina";
        case pizza::PizzaType::Margarita: return "margarita";
        case pizza::PizzaType::Americana: return "americana";
        case pizza::PizzaType::Fantasia:  return "fantasia";
    }
    return "?";
}

std::string sizeName(pizza::PizzaSize size)
{
    switch (size) {
        case pizza::PizzaSize::S:   return "S";
        case pizza::PizzaSize::M:   return "M";
        case pizza::PizzaSize::L:   return "L";
        case pizza::PizzaSize::XL:  return "XL";
        case pizza::PizzaSize::XXL: return "XXL";
    }
    return "?";
}

}

Reception::Reception(ReceptionConfig config, LogSink log)
    : _config(config), _log(std::move(log)),
      _spawner(config.kitchenDefaults),
      _dispatcher({},
          [this]() -> KitchenHandle * { return spawnKitchen(); }),
      _reaper([this](pid_t pid, concurrency::ExitStatus status) { onChildExit(pid, status); },
          config.pollInterval),
      _nextKitchenId(1),
      _shutdownRequested(false), _running(false),
      _dispatched(0), _completed(0)
{
}

Reception::~Reception()
{
    requestShutdown();
    shutdownAll();
    if (_config.installSignalHandler)
        SignalHandler::uninstall();
}

void Reception::run(std::istream &input)
{
    if (_running.exchange(true))
        throw Error("Reception::run: already running");

    if (_config.installSignalHandler) {
        SignalHandler::reset();
        SignalHandler::install();
    }
    _reaper.start();

    ShellCallbacks callbacks;
    callbacks.onOrder  = [this](const parser::ParsedOrder &o) { onOrder(o); };
    callbacks.onStatus = [this] { onStatusCommand(); };
    callbacks.onError  = [this](const std::string &what) { log("parse error: " + what); };
    _shell = std::make_unique<Shell>(input, std::move(callbacks));
    _shell->start();

    log("reception started");

    while (true) {
        if (_shutdownRequested.load())
            break;
        if (_config.installSignalHandler && SignalHandler::requested()) {
            log("SIGINT received, shutting down");
            break;
        }
        if (_shell->finished())
            break;
        std::this_thread::sleep_for(_config.pollInterval);
    }

    _shell->requestStop();
    _shell->join();

    const auto drainDeadline = std::chrono::steady_clock::now() + _config.drainTimeout;
    while (_completed.load() < _dispatched.load()) {
        if (_config.installSignalHandler && SignalHandler::requested())
            break;
        if (std::chrono::steady_clock::now() >= drainDeadline) {
            log("drain timeout reached, some pizzas may be incomplete");
            break;
        }
        std::this_thread::sleep_for(_config.pollInterval);
    }
    shutdownAll();
    _reaper.stop();
    _running.store(false);
    log("reception stopped");
}

void Reception::requestShutdown()
{
    _shutdownRequested.store(true);
}

std::size_t Reception::kitchenCount() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _kitchens.size();
}

void Reception::onOrder(const parser::ParsedOrder &order)
{
    try {
        auto ids = _dispatcher.dispatch(order);
        _dispatched.fetch_add(ids.size());
        std::ostringstream oss;
        oss << "dispatched " << ids.size() << " x " << pizzaName(order.type)
            << " " << sizeName(order.size);
        log(oss.str());
    } catch (const std::exception &e) {
        log(std::string("dispatch failed: ") + e.what());
    }
}

void Reception::onStatusCommand()
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    std::ostringstream oss;
    oss << "status: " << _kitchens.size() << " kitchens";
    for (const auto &entry : _kitchens) {
        oss << " | k" << entry->handle->id()
            << " inFlight=" << entry->handle->inFlight()
            << "/" << entry->handle->maxConcurrent();
        try {
            entry->handle->sendStatusRequest();
        } catch (...) {
        }
    }
    log(oss.str());
}

KitchenHandle *Reception::spawnKitchen()
{
    std::int32_t newId;
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        newId = _nextKitchenId++;
    }
    auto spawned = _spawner.spawn(newId);
    pid_t pid = spawned.process.pid();
    auto entry = std::make_unique<KitchenEntry>();
    entry->handle  = std::move(spawned.handle);
    entry->process = std::move(spawned.process);
    KitchenHandle *raw = entry->handle.get();

    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        _kitchens.push_back(std::move(entry));
        startReaderFor(*_kitchens.back());
    }
    _reaper.track(pid);
    rebuildDispatcherHandles();

    std::ostringstream oss;
    oss << "spawned kitchen k" << newId << " pid=" << pid;
    log(oss.str());
    return raw;
}

void Reception::startReaderFor(KitchenEntry &entry)
{
    KitchenEntry *entryPtr = &entry;
    entry.reader = concurrency::Thread([this, entryPtr] {
        while (!entryPtr->readerStop.load()) {
            std::optional<ipc::Message> msg;
            try {
                msg = entryPtr->handle->tryReceive(std::chrono::milliseconds(100));
            } catch (...) {
                msg = std::nullopt;
            }
            if (msg.has_value())
                onIncoming(*entryPtr, *msg);
            else if (!entryPtr->handle->isOpen())
                return;
        }
    });
}

void Reception::onIncoming(KitchenEntry &entry, const ipc::Message &message)
{
    std::visit([&](auto &&value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, ipc::PizzaDoneMsg>) {
            entry.handle->noteOrderCompleted();
            _completed.fetch_add(1);
            std::ostringstream oss;
            oss << "pizza done order=" << value.orderId << " kitchen=" << value.kitchenId;
            log(oss.str());
        } else if constexpr (std::is_same_v<T, ipc::StatusReplyMsg>) {
            std::ostringstream oss;
            oss << "k" << value.kitchenId << " status: busy=" << value.cooksBusy
                << "/" << value.cooksTotal << " queue=" << value.queueSize;
            log(oss.str());
        }
    }, message);
}

void Reception::onChildExit(pid_t pid, concurrency::ExitStatus status)
{
    std::ostringstream oss;
    oss << "child pid=" << pid;
    if (status.exited)
        oss << " exited code=" << status.code;
    else if (status.signaled)
        oss << " killed by signal=" << status.signal;
    log(oss.str());
}

void Reception::shutdownAll()
{
    std::vector<std::unique_ptr<KitchenEntry>> drained;
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        drained.swap(_kitchens);
    }
    for (auto &entry : drained) {
        try {
            entry->handle->sendShutdown();
        } catch (...) {
        }
    }
    for (auto &entry : drained) {
        entry->readerStop.store(true);
        if (entry->reader.joinable())
            entry->reader.join();
        entry->handle->close();
        if (entry->process.joinable()) {
            try {
                (void)entry->process.wait();
            } catch (...) {
            }
        }
    }
    rebuildDispatcherHandles();
}

void Reception::log(const std::string &msg) const
{
    if (_log)
        _log(msg);
}

std::vector<KitchenHandle *> Reception::handlesSnapshot() const
{
    std::vector<KitchenHandle *> out;
    out.reserve(_kitchens.size());
    for (const auto &entry : _kitchens)
        out.push_back(entry->handle.get());
    return out;
}

void Reception::rebuildDispatcherHandles()
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    _dispatcher.setHandles(handlesSnapshot());
}

}
