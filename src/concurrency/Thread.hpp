#pragma once

#include <thread>
#include <type_traits>
#include <utility>

namespace plazza::concurrency {

class Thread {
public:
    using id = std::thread::id;
    using native_handle_type = std::thread::native_handle_type;

    Thread() noexcept = default;

    template <typename Callable, typename... Args,
        typename = std::enable_if_t<!std::is_same_v<std::decay_t<Callable>, Thread>>>
    explicit Thread(Callable &&callable, Args &&...args)
        : _thread(std::forward<Callable>(callable), std::forward<Args>(args)...)
    {
    }

    ~Thread();

    Thread(const Thread &) = delete;
    Thread &operator=(const Thread &) = delete;

    Thread(Thread &&other) noexcept = default;
    Thread &operator=(Thread &&other) noexcept;

    [[nodiscard]] bool joinable() const noexcept { return _thread.joinable(); }
    void join();
    void detach();
    void swap(Thread &other) noexcept { _thread.swap(other._thread); }

    [[nodiscard]] id get_id() const noexcept { return _thread.get_id(); }
    [[nodiscard]] native_handle_type native_handle() { return _thread.native_handle(); }

    static unsigned int hardware_concurrency() noexcept { return std::thread::hardware_concurrency(); }

private:
    std::thread _thread;
};

}
