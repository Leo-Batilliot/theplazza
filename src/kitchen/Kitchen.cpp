#include "kitchen/Kitchen.hpp"

#include "common/Errors.hpp"
#include "ipc/Serializer.hpp"
#include "pizza/Pizza.hpp"
#include "pizza/PizzaFactory.hpp"

#include <memory>
#include <mutex>
#include <utility>
#include <variant>

namespace plazza::kitchen {

namespace {

constexpr std::chrono::milliseconds kIdlePollInterval{200};

}

Kitchen::Kitchen(KitchenConfig config)
    : _config(config),
      _stock(config.stockInitial),
      _pool(config.maxConcurrentPizzas()),
      _capacity(config.maxConcurrentPizzas()),
      _idle(config.idleTimeout),
      _cook(_stock, config.timeMultiplier),
      _restocker(_stock, config.restockPeriod),
      _shutdownRequested(false),
      _channelClosed(false),
      _running(false)
{
    if (config.cookCount == 0)
        throw KitchenError("Kitchen: cookCount must be > 0");
}

Kitchen::~Kitchen()
{
    _restocker.stop();
    _stock.shutdown();
    _pool.shutdown();
}

void Kitchen::run(ipc::IpcChannel &channel)
{
    if (_running.exchange(true))
        throw KitchenError("Kitchen::run: already running");

    _restocker.start();
    concurrency::Thread reader([this, &channel] { readerLoop(channel); });

    while (true) {
        std::queue<ipc::Message> drained;
        bool stop = false;
        {
            std::unique_lock<concurrency::Mutex> lock(_stateMutex);
            _stateCv.wait_for(lock, kIdlePollInterval, [this] {
                return !_incoming.empty() || _shutdownRequested || _channelClosed;
            });
            drained.swap(_incoming);
            if (_shutdownRequested)
                stop = true;
        }

        while (!drained.empty()) {
            handleIncoming(channel, drained.front());
            drained.pop();
        }

        if (stop)
            break;

        if (_channelClosed && _capacity.inUse() == 0)
            break;
        if (_idle.hasExpired() && _capacity.inUse() == 0)
            break;
    }

    _running.store(false);
    _restocker.stop();
    _stock.shutdown();
    _pool.shutdown();
    channel.close();
    if (reader.joinable())
        reader.join();
}

void Kitchen::readerLoop(ipc::IpcChannel &channel)
{
    while (true) {
        std::optional<std::vector<std::uint8_t>> frame;
        try {
            frame = channel.receive();
        } catch (const IpcError &) {
            frame = std::nullopt;
        }
        if (!frame) {
            std::lock_guard<concurrency::Mutex> lock(_stateMutex);
            _channelClosed = true;
            _stateCv.notify_all();
            return;
        }
        ipc::Deserializer in(std::move(*frame));
        ipc::Message message = ipc::decode(in);
        {
            std::lock_guard<concurrency::Mutex> lock(_stateMutex);
            _incoming.push(std::move(message));
        }
        _stateCv.notify_all();
    }
}

void Kitchen::handleIncoming(ipc::IpcChannel &channel, const ipc::Message &message)
{
    _idle.notifyActivity();
    std::visit([&](auto &&value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, ipc::OrderMsg>) {
            handleOrder(channel, value);
        } else if constexpr (std::is_same_v<T, ipc::StatusRequestMsg>) {
            handleStatusRequest(channel);
        } else if constexpr (std::is_same_v<T, ipc::ShutdownMsg>) {
            std::lock_guard<concurrency::Mutex> lock(_stateMutex);
            _shutdownRequested = true;
            _stateCv.notify_all();
        }
    }, message);
}

void Kitchen::handleOrder(ipc::IpcChannel &channel, const ipc::OrderMsg &order)
{
    if (!_capacity.tryAcquire())
        return;
    _idle.enter();

    auto pizza = pizza::PizzaFactory::instance().create(order.type, order.size);
    const std::int32_t kitchenId = _config.id;
    const std::uint64_t orderId = order.orderId;

    std::shared_ptr<pizza::Pizza> sharedPizza(std::move(pizza));

    _pool.submit([this, &channel, kitchenId, orderId, sharedPizza] {
        bool ok = false;
        try {
            ok = _cook.cook(*sharedPizza);
        } catch (...) {
            ok = false;
        }
        if (ok) {
            try {
                sendLocked(channel, ipc::Message{ipc::PizzaDoneMsg{orderId, kitchenId}});
            } catch (...) {
            }
        }
        _capacity.release();
        _idle.leave();
        _stateCv.notify_all();
    });
}

void Kitchen::handleStatusRequest(ipc::IpcChannel &channel)
{
    ipc::StatusReplyMsg reply{};
    reply.kitchenId  = _config.id;
    reply.cooksTotal = static_cast<std::uint32_t>(_config.cookCount);
    reply.cooksBusy  = static_cast<std::uint32_t>(_capacity.inUse());
    reply.queueSize  = static_cast<std::uint32_t>(_pool.pending());
    reply.stock      = _stock.snapshot();
    try {
        sendLocked(channel, ipc::Message{reply});
    } catch (...) {
    }
}

void Kitchen::sendLocked(ipc::IpcChannel &channel, const ipc::Message &message)
{
    ipc::Serializer out;
    ipc::encode(out, message);
    std::lock_guard<concurrency::Mutex> lock(_sendMutex);
    if (!channel.isOpen())
        return;
    channel.send(out.take());
}

}
