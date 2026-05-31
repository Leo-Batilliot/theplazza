#pragma once

#include "concurrency/Mutex.hpp"
#include "concurrency/Process.hpp"
#include "concurrency/Thread.hpp"

#include <sys/types.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <unordered_map>

namespace plazza::reception {

class ChildReaper {
public:
    using OnExit = std::function<void(pid_t pid, concurrency::ExitStatus status)>;

    explicit ChildReaper(OnExit onExit,
        std::chrono::milliseconds pollInterval = std::chrono::milliseconds(50));
    ~ChildReaper();

    ChildReaper(const ChildReaper &) = delete;
    ChildReaper &operator=(const ChildReaper &) = delete;

    void track(pid_t pid);
    void untrack(pid_t pid);

    void start();
    void stop();
    [[nodiscard]] bool running() const noexcept { return _running.load(); }

private:
    void loop();
    void pollOnce();

    OnExit _onExit;
    std::chrono::milliseconds _pollInterval;
    mutable concurrency::Mutex _mutex;
    std::unordered_map<pid_t, bool> _tracked;
    std::atomic<bool> _running;
    std::atomic<bool> _stopRequested;
    concurrency::Thread _thread;
};

}
