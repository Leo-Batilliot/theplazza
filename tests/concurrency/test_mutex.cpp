#include <criterion/criterion.h>

#include "concurrency/Mutex.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

using plazza::concurrency::Mutex;

Test(mutex_basic, lock_then_unlock_succeeds)
{
    Mutex m;

    m.lock();
    m.unlock();
    cr_assert(true, "lock/unlock cycle completed without throwing");
}

Test(mutex_basic, try_lock_on_free_mutex_succeeds)
{
    Mutex m;

    cr_assert(m.try_lock(), "try_lock on a free mutex must return true");
    m.unlock();
}

Test(mutex_basic, try_lock_on_held_mutex_fails_from_other_thread)
{
    Mutex m;
    std::atomic<bool> got_lock{true};

    m.lock();
    std::thread t([&] { got_lock.store(m.try_lock()); });
    t.join();
    m.unlock();

    cr_assert_not(got_lock.load(), "try_lock from another thread must fail while the mutex is held");
}

Test(mutex_traits, is_non_copyable_and_non_movable)
{
    static_assert(!std::is_copy_constructible_v<Mutex>, "Mutex must not be copy-constructible");
    static_assert(!std::is_copy_assignable_v<Mutex>, "Mutex must not be copy-assignable");
    static_assert(!std::is_move_constructible_v<Mutex>, "Mutex must not be move-constructible");
    static_assert(!std::is_move_assignable_v<Mutex>, "Mutex must not be move-assignable");
    cr_assert(true);
}

Test(mutex_stl, works_with_lock_guard)
{
    Mutex m;

    {
        std::lock_guard<Mutex> guard(m);
        cr_assert_not(m.try_lock(), "mutex must be held while lock_guard is alive");
    }
    cr_assert(m.try_lock(), "mutex must be released after lock_guard destruction");
    m.unlock();
}

Test(mutex_stl, works_with_unique_lock_defer)
{
    Mutex m;

    std::unique_lock<Mutex> lk(m, std::defer_lock);
    cr_assert_not(lk.owns_lock(), "defer_lock must not acquire the mutex");
    lk.lock();
    cr_assert(lk.owns_lock(), "explicit lock() through unique_lock must succeed");
    lk.unlock();
    cr_assert_not(lk.owns_lock(), "explicit unlock() through unique_lock must release ownership");
}

Test(mutex_stl, works_with_unique_lock_try_to_lock)
{
    Mutex m;

    m.lock();
    std::atomic<bool> owned{true};
    std::thread t([&] {
        std::unique_lock<Mutex> lk(m, std::try_to_lock);
        owned.store(lk.owns_lock());
    });
    t.join();
    m.unlock();

    cr_assert_not(owned.load(), "try_to_lock unique_lock must not acquire a held mutex");
}

Test(mutex_concurrent, mutual_exclusion_keeps_counter_correct, .timeout = 10)
{
    constexpr int thread_count = 8;
    constexpr int iterations = 5000;
    Mutex m;
    long long counter = 0;

    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        workers.emplace_back([&] {
            for (int j = 0; j < iterations; ++j) {
                std::lock_guard<Mutex> guard(m);
                ++counter;
            }
        });
    }
    for (auto &t : workers)
        t.join();

    cr_assert_eq(counter, static_cast<long long>(thread_count) * iterations,
        "mutex must serialize increments — got %lld, expected %lld",
        counter, static_cast<long long>(thread_count) * iterations);
}

Test(mutex_native, native_handle_is_usable)
{
    Mutex m;

    auto handle = m.native_handle();
    cr_assert(handle != Mutex::native_handle_type{}, "native_handle should expose a non-default OS handle");
}
