#pragma once

#include <mutex>

namespace plazza::concurrency {

class Mutex {
public:
    Mutex() = default;
    ~Mutex() = default;

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;
    Mutex(Mutex &&) = delete;
    Mutex &operator=(Mutex &&) = delete;

    void lock();
    void unlock();
    [[nodiscard]] bool try_lock();

    using native_handle_type = std::mutex::native_handle_type;
    [[nodiscard]] native_handle_type native_handle() noexcept { return _mutex.native_handle(); }

private:
    std::mutex _mutex;
};

}
