#pragma once

#include "concurrency/ConditionVar.hpp"
#include "concurrency/Mutex.hpp"
#include "concurrency/Thread.hpp"
#include "ipc/IpcChannel.hpp"
#include "ipc/Messages.hpp"
#include "kitchen/Capacity.hpp"
#include "kitchen/Cook.hpp"
#include "kitchen/IdleMonitor.hpp"
#include "kitchen/Restocker.hpp"
#include "kitchen/Stock.hpp"
#include "kitchen/ThreadPool.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <queue>

namespace plazza::kitchen {

struct KitchenConfig {
    std::int32_t id = 0;
    std::size_t cookCount = 2;
    double timeMultiplier = 1.0;
    std::chrono::milliseconds restockPeriod{1000};
    std::chrono::milliseconds idleTimeout{5000};
    std::uint32_t stockInitial = Stock::initialCount;

    [[nodiscard]] std::size_t maxConcurrentPizzas() const noexcept { return cookCount * 2; }
};

class Kitchen {
public:
    explicit Kitchen(KitchenConfig config);
    ~Kitchen();

    Kitchen(const Kitchen &) = delete;
    Kitchen &operator=(const Kitchen &) = delete;
    Kitchen(Kitchen &&) = delete;
    Kitchen &operator=(Kitchen &&) = delete;

    void run(ipc::IpcChannel &channel);

    [[nodiscard]] const KitchenConfig &config() const noexcept { return _config; }
    [[nodiscard]] Stock &stock() noexcept { return _stock; }
    [[nodiscard]] Capacity &capacity() noexcept { return _capacity; }
    [[nodiscard]] IdleMonitor &idle() noexcept { return _idle; }

private:
    void readerLoop(ipc::IpcChannel &channel);
    void handleIncoming(ipc::IpcChannel &channel, const ipc::Message &message);
    void handleOrder(ipc::IpcChannel &channel, const ipc::OrderMsg &order);
    void handleStatusRequest(ipc::IpcChannel &channel);
    void sendLocked(ipc::IpcChannel &channel, const ipc::Message &message);

    KitchenConfig _config;
    Stock _stock;
    ThreadPool _pool;
    Capacity _capacity;
    IdleMonitor _idle;
    Cook _cook;
    Restocker _restocker;

    concurrency::Mutex _stateMutex;
    concurrency::ConditionVar _stateCv;
    std::queue<ipc::Message> _incoming;
    bool _shutdownRequested;
    bool _channelClosed;

    concurrency::Mutex _sendMutex;
    std::atomic<bool> _running;
    std::atomic<bool> _readerStop;
};

}
