#include "kitchen/ThreadPool.hpp"

#include "common/Errors.hpp"

#include <mutex>
#include <utility>

namespace plazza::kitchen {

ThreadPool::ThreadPool(std::size_t workerCount)
    : _active(0), _state(State::Running)
{
    if (workerCount == 0)
        throw KitchenError("ThreadPool: worker count must be > 0");
    _workers.reserve(workerCount);
    for (std::size_t i = 0; i < workerCount; ++i)
        _workers.emplace_back([this] { workerLoop(); });
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

void ThreadPool::submit(Task task)
{
    if (!task)
        throw KitchenError("ThreadPool::submit: empty task");
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        if (_state != State::Running)
            throw KitchenError("ThreadPool::submit: pool is shut down");
        _queue.push(std::move(task));
    }
    _condition.notify_one();
}

void ThreadPool::shutdown()
{
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        if (_state == State::Cancelled)
            return;
        if (_state == State::Running)
            _state = State::Draining;
    }
    _condition.notify_all();
    for (auto &worker : _workers) {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::shutdownNow()
{
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        _state = State::Cancelled;
        std::queue<Task> empty;
        _queue.swap(empty);
    }
    _condition.notify_all();
    for (auto &worker : _workers) {
        if (worker.joinable())
            worker.join();
    }
}

std::size_t ThreadPool::pending() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _queue.size();
}

std::size_t ThreadPool::activeWorkers() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _active;
}

bool ThreadPool::isShutdown() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _state != State::Running;
}

void ThreadPool::workerLoop()
{
    while (true) {
        Task task;
        {
            std::unique_lock<concurrency::Mutex> lock(_mutex);
            _condition.wait(lock, [this] {
                return !_queue.empty() || _state != State::Running;
            });
            if (_state == State::Cancelled)
                return;
            if (_queue.empty())
                return;
            task = std::move(_queue.front());
            _queue.pop();
            ++_active;
        }
        try {
            task();
        } catch (...) {
        }
        {
            std::lock_guard<concurrency::Mutex> lock(_mutex);
            --_active;
        }
    }
}

}
