#pragma once

#include "concurrency/Mutex.hpp"

#include <chrono>
#include <cstddef>

namespace plazza::kitchen {

class IdleMonitor {
public:
    using clock = std::chrono::steady_clock;
    using duration = std::chrono::milliseconds;

    explicit IdleMonitor(duration threshold);
    ~IdleMonitor() = default;

    IdleMonitor(const IdleMonitor &) = delete;
    IdleMonitor &operator=(const IdleMonitor &) = delete;
    IdleMonitor(IdleMonitor &&) = delete;
    IdleMonitor &operator=(IdleMonitor &&) = delete;

    void notifyActivity();
    void enter();
    void leave();

    [[nodiscard]] bool isBusy() const;
    [[nodiscard]] std::size_t busyCount() const;
    [[nodiscard]] duration idleDuration() const;
    [[nodiscard]] bool hasExpired() const;
    [[nodiscard]] duration threshold() const noexcept { return _threshold; }

private:
    [[nodiscard]] duration idleDurationLocked() const;

    mutable concurrency::Mutex _mutex;
    duration _threshold;
    std::size_t _busy;
    clock::time_point _lastActivity;
};

}
