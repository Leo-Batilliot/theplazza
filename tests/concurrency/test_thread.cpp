#include <criterion/criterion.h>

#include "concurrency/Thread.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using plazza::concurrency::Thread;
using namespace std::chrono_literals;

Test(thread_traits, is_non_copyable_and_movable)
{
    static_assert(!std::is_copy_constructible_v<Thread>, "Thread must not be copy-constructible");
    static_assert(!std::is_copy_assignable_v<Thread>, "Thread must not be copy-assignable");
    static_assert(std::is_move_constructible_v<Thread>, "Thread must be move-constructible");
    static_assert(std::is_move_assignable_v<Thread>, "Thread must be move-assignable");
    cr_assert(true);
}

Test(thread_basic, default_constructed_is_not_joinable)
{
    Thread t;
    cr_assert_not(t.joinable(), "a default-constructed Thread must not be joinable");
}

Test(thread_basic, constructed_with_callable_is_joinable)
{
    Thread t([] {});
    cr_assert(t.joinable(), "a Thread built from a callable must be joinable until joined/detached");
    t.join();
    cr_assert_not(t.joinable(), "Thread must not be joinable after join()");
}

Test(thread_basic, join_waits_for_completion, .timeout = 5)
{
    std::atomic<bool> done{false};
    Thread t([&] {
        std::this_thread::sleep_for(20ms);
        done.store(true);
    });
    t.join();
    cr_assert(done.load(), "join() must block until the thread's callable has returned");
}

Test(thread_basic, detach_releases_ownership, .timeout = 5)
{
    std::atomic<bool> ran{false};
    Thread t([&] {
        std::this_thread::sleep_for(10ms);
        ran.store(true);
    });
    t.detach();
    cr_assert_not(t.joinable(), "Thread must not be joinable after detach()");
    for (int i = 0; i < 100 && !ran.load(); ++i)
        std::this_thread::sleep_for(5ms);
    cr_assert(ran.load(), "detached thread must still execute its callable");
}

Test(thread_args, forwards_variadic_arguments, .timeout = 5)
{
    std::atomic<int> sum{0};
    Thread t([&](int a, int b, int c) { sum.store(a + b + c); }, 1, 2, 3);
    t.join();
    cr_assert_eq(sum.load(), 6, "variadic arguments must be forwarded to the callable");
}

Test(thread_id, get_id_matches_self_inside_callable, .timeout = 5)
{
    Thread::id observed{};
    Thread t([&] { observed = std::this_thread::get_id(); });
    Thread::id expected = t.get_id();
    t.join();
    cr_assert(observed == expected, "Thread::get_id() must match std::this_thread::get_id() inside the callable");
}

Test(thread_id, default_thread_id_is_empty)
{
    Thread t;
    cr_assert(t.get_id() == Thread::id{}, "default-constructed Thread must have a default id");
}

Test(thread_move, move_construction_transfers_ownership, .timeout = 5)
{
    std::atomic<bool> done{false};
    Thread source([&] {
        std::this_thread::sleep_for(10ms);
        done.store(true);
    });
    Thread::id source_id = source.get_id();

    Thread sink(std::move(source));
    cr_assert_not(source.joinable(), "moved-from Thread must not be joinable");
    cr_assert(sink.joinable(), "moved-to Thread must own the underlying thread");
    cr_assert(sink.get_id() == source_id, "moved-to Thread must inherit the original thread id");

    sink.join();
    cr_assert(done.load(), "callable must complete on the moved-to Thread");
}

Test(thread_move, move_assignment_joins_existing_thread, .timeout = 5)
{
    std::atomic<int> finished{0};
    Thread lhs([&] {
        std::this_thread::sleep_for(5ms);
        finished.fetch_add(1);
    });
    Thread rhs([&] {
        std::this_thread::sleep_for(200ms);
        finished.fetch_add(10);
    });

    lhs = std::move(rhs);

    cr_assert_eq(finished.load(), 1,
        "move-assignment must join the previously-owned (fast) thread before taking ownership");
    cr_assert(lhs.joinable(), "lhs must now own the rhs thread");
    cr_assert_not(rhs.joinable(), "rhs must be empty after being moved from");

    lhs.join();
    cr_assert_eq(finished.load(), 11, "rhs callable must still run after the move");
}

Test(thread_move, self_move_assignment_is_safe, .timeout = 5)
{
    Thread t([] { std::this_thread::sleep_for(5ms); });
    Thread &ref = t;
    t = std::move(ref);
    cr_assert(t.joinable(), "self move-assignment must not destroy the running thread");
    t.join();
}

Test(thread_raii, destructor_joins_running_thread, .timeout = 5)
{
    std::atomic<bool> done{false};
    {
        Thread t([&] {
            std::this_thread::sleep_for(20ms);
            done.store(true);
        });
    }
    cr_assert(done.load(), "Thread destructor must join a still-running thread (RAII)");
}

Test(thread_raii, destructor_on_non_joinable_is_noop)
{
    Thread t;
    cr_assert_not(t.joinable());
}

Test(thread_concurrent, many_threads_increment_counter_correctly, .timeout = 10)
{
    constexpr int thread_count = 16;
    constexpr int iterations = 1000;
    std::atomic<long long> counter{0};

    std::vector<Thread> workers;
    workers.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i)
        workers.emplace_back([&] {
            for (int j = 0; j < iterations; ++j)
                counter.fetch_add(1, std::memory_order_relaxed);
        });
    for (auto &t : workers)
        t.join();

    cr_assert_eq(counter.load(), static_cast<long long>(thread_count) * iterations,
        "all spawned Threads must complete and contribute their increments");
}

Test(thread_swap, member_swap_exchanges_ownership, .timeout = 5)
{
    std::atomic<int> who{0};
    Thread a([&] { who.store(1); });
    Thread b;

    Thread::id a_id = a.get_id();
    a.swap(b);

    cr_assert_not(a.joinable(), "after swap, a must be empty");
    cr_assert(b.joinable(), "after swap, b must own the original a thread");
    cr_assert(b.get_id() == a_id, "after swap, b must hold the original thread id");
    b.join();
    cr_assert_eq(who.load(), 1);
}

Test(thread_static, hardware_concurrency_is_consistent_with_std)
{
    cr_assert_eq(Thread::hardware_concurrency(), std::thread::hardware_concurrency(),
        "Thread::hardware_concurrency must mirror std::thread::hardware_concurrency");
}

Test(thread_native, native_handle_exposes_underlying_handle, .timeout = 5)
{
    Thread t([] { std::this_thread::sleep_for(5ms); });
    Thread::native_handle_type handle = t.native_handle();
    cr_assert(handle != Thread::native_handle_type{}, "running Thread must expose a non-default native handle");
    t.join();
}
