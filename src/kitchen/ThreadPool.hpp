#pragma once

#include "concurrency/ConditionVar.hpp"
#include "concurrency/Mutex.hpp"
#include "concurrency/Thread.hpp"

#include <cstddef>
#include <functional>
#include <queue>
#include <vector>

namespace plazza::kitchen {

class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(std::size_t workerCount);
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    void submit(Task task);
    void shutdown();
    void shutdownNow();

    [[nodiscard]] std::size_t size() const noexcept { return _workers.size(); }
    [[nodiscard]] std::size_t pending() const;
    [[nodiscard]] std::size_t activeWorkers() const;
    [[nodiscard]] bool isShutdown() const;

private:
    enum class State { Running, Draining, Cancelled };

    void workerLoop();

    mutable concurrency::Mutex _mutex;
    concurrency::ConditionVar _condition;
    std::queue<Task> _queue;
    std::vector<concurrency::Thread> _workers;
    std::size_t _active;
    State _state;
};

}
