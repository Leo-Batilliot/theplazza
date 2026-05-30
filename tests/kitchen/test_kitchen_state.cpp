#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "kitchen/Capacity.hpp"
#include "kitchen/IdleMonitor.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using plazza::KitchenError;
using plazza::kitchen::Capacity;
using plazza::kitchen::IdleMonitor;
using namespace std::chrono_literals;

Test(capacity_basic, zero_max_is_rejected)
{
    cr_assert_throw(Capacity{0}, KitchenError);
}

Test(capacity_basic, initial_state_is_empty)
{
    Capacity cap(4);
    cr_assert_eq(cap.max(), 4u);
    cr_assert_eq(cap.inUse(), 0u);
    cr_assert_eq(cap.free(), 4u);
    cr_assert_not(cap.full());
}

Test(capacity_acquire, try_acquire_until_full_then_fails)
{
    Capacity cap(3);
    cr_assert(cap.tryAcquire());
    cr_assert(cap.tryAcquire());
    cr_assert(cap.tryAcquire());
    cr_assert(cap.full());
    cr_assert_not(cap.tryAcquire());
    cr_assert_eq(cap.inUse(), 3u);
    cr_assert_eq(cap.free(), 0u);
}

Test(capacity_release, release_frees_a_slot)
{
    Capacity cap(2);
    cr_assert(cap.tryAcquire());
    cr_assert(cap.tryAcquire());
    cap.release();
    cr_assert_eq(cap.inUse(), 1u);
    cr_assert(cap.tryAcquire());
    cr_assert(cap.full());
}

Test(capacity_release, underflow_throws)
{
    Capacity cap(2);
    cr_assert_throw(cap.release(), KitchenError);
}

Test(capacity_concurrent, never_exceeds_max_under_contention, .timeout = 5)
{
    constexpr std::size_t max = 4;
    Capacity cap(max);
    constexpr int workers = 8;
    constexpr int perThread = 200;
    std::atomic<int> oversubscribed{0};

    std::vector<std::thread> threads;
    threads.reserve(workers);
    for (int t = 0; t < workers; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < perThread; ++i) {
                if (!cap.tryAcquire())
                    continue;
                if (cap.inUse() > max)
                    oversubscribed.fetch_add(1);
                cap.release();
            }
        });
    }
    for (auto &th : threads)
        th.join();

    cr_assert_eq(oversubscribed.load(), 0);
    cr_assert_eq(cap.inUse(), 0u);
}

Test(idle_basic, non_positive_threshold_rejected)
{
    cr_assert_throw(IdleMonitor{0ms}, KitchenError);
    cr_assert_throw(IdleMonitor{std::chrono::milliseconds(-1)}, KitchenError);
}

Test(idle_basic, fresh_monitor_is_not_busy)
{
    IdleMonitor monitor(100ms);
    cr_assert_not(monitor.isBusy());
    cr_assert_eq(monitor.busyCount(), 0u);
}

Test(idle_busy_tracking, enter_leave_reflect_busy_count)
{
    IdleMonitor monitor(100ms);
    monitor.enter();
    monitor.enter();
    cr_assert(monitor.isBusy());
    cr_assert_eq(monitor.busyCount(), 2u);
    monitor.leave();
    cr_assert_eq(monitor.busyCount(), 1u);
    monitor.leave();
    cr_assert_not(monitor.isBusy());
}

Test(idle_busy_tracking, leave_underflow_throws)
{
    IdleMonitor monitor(100ms);
    cr_assert_throw(monitor.leave(), KitchenError);
}

Test(idle_expiry, idle_duration_zero_while_busy, .timeout = 5)
{
    IdleMonitor monitor(50ms);
    monitor.enter();
    std::this_thread::sleep_for(80ms);
    cr_assert_eq(monitor.idleDuration().count(), 0);
    cr_assert_not(monitor.hasExpired());
    monitor.leave();
}

Test(idle_expiry, becomes_expired_after_threshold, .timeout = 5)
{
    IdleMonitor monitor(40ms);
    cr_assert_not(monitor.hasExpired());
    std::this_thread::sleep_for(80ms);
    cr_assert(monitor.hasExpired());
    cr_assert_geq(monitor.idleDuration().count(), 40);
}

Test(idle_expiry, activity_resets_idle_clock, .timeout = 5)
{
    IdleMonitor monitor(50ms);
    std::this_thread::sleep_for(30ms);
    monitor.notifyActivity();
    std::this_thread::sleep_for(30ms);
    cr_assert_not(monitor.hasExpired(), "activity should have reset idle clock");
    std::this_thread::sleep_for(40ms);
    cr_assert(monitor.hasExpired());
}

Test(idle_expiry, idle_clock_restarts_when_last_task_leaves, .timeout = 5)
{
    IdleMonitor monitor(40ms);
    std::this_thread::sleep_for(60ms);
    monitor.enter();
    std::this_thread::sleep_for(60ms);
    monitor.leave();
    cr_assert_not(monitor.hasExpired(), "leave() must reset the idle clock");
    std::this_thread::sleep_for(60ms);
    cr_assert(monitor.hasExpired());
}
