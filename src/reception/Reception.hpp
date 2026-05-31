#pragma once

#include "concurrency/Mutex.hpp"
#include "concurrency/Process.hpp"
#include "concurrency/Thread.hpp"
#include "ipc/Messages.hpp"
#include "kitchen/Kitchen.hpp"
#include "parser/Parser.hpp"
#include "reception/ChildReaper.hpp"
#include "reception/Dispatcher.hpp"
#include "reception/KitchenHandle.hpp"
#include "reception/Shell.hpp"
#include "reception/Spawner.hpp"

#include <sys/types.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <vector>

namespace plazza::reception {

struct ReceptionConfig {
    kitchen::KitchenConfig kitchenDefaults;
    std::chrono::milliseconds pollInterval{50};
    std::chrono::milliseconds drainTimeout{10000};
    bool installSignalHandler = true;
};

class Reception {
public:
    using LogSink = std::function<void(const std::string &)>;

    explicit Reception(ReceptionConfig config, LogSink log = nullptr);
    ~Reception();

    Reception(const Reception &) = delete;
    Reception &operator=(const Reception &) = delete;

    void run(std::istream &input);
    void requestShutdown();

    [[nodiscard]] std::size_t kitchenCount() const;
    [[nodiscard]] std::uint64_t totalDispatched() const noexcept { return _dispatched.load(); }
    [[nodiscard]] std::uint64_t totalCompleted() const noexcept { return _completed.load(); }

private:
    struct KitchenEntry {
        std::unique_ptr<KitchenHandle> handle;
        concurrency::Process process;
        concurrency::Thread reader;
        std::atomic<bool> readerStop{false};
    };

    void onOrder(const parser::ParsedOrder &order);
    void onStatusCommand();
    KitchenHandle *spawnKitchen();
    void startReaderFor(KitchenEntry &entry);
    void onIncoming(KitchenEntry &entry, const ipc::Message &message);
    void onChildExit(pid_t pid, concurrency::ExitStatus status);
    void shutdownAll();
    void log(const std::string &msg) const;
    [[nodiscard]] std::vector<KitchenHandle *> handlesSnapshot() const;
    void rebuildDispatcherHandles();

    ReceptionConfig _config;
    LogSink _log;

    Spawner _spawner;
    Dispatcher _dispatcher;
    ChildReaper _reaper;
    std::unique_ptr<Shell> _shell;

    mutable concurrency::Mutex _mutex;
    std::vector<std::unique_ptr<KitchenEntry>> _kitchens;
    std::int32_t _nextKitchenId;

    std::atomic<bool> _shutdownRequested;
    std::atomic<bool> _running;
    std::atomic<std::uint64_t> _dispatched;
    std::atomic<std::uint64_t> _completed;
};

}
