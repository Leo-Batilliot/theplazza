#include <criterion/criterion.h>

#include "concurrency/ConditionVar.hpp"
#include "concurrency/Mutex.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

using plazza::concurrency::ConditionVar;
using plazza::concurrency::Mutex;
using namespace std::chrono_literals;

Test(condvar_traits, is_non_copyable_and_non_movable)
{
    static_assert(!std::is_copy_constructible_v<ConditionVar>, "ConditionVar must not be copy-constructible");
    static_assert(!std::is_copy_assignable_v<ConditionVar>, "ConditionVar must not be copy-assignable");
    static_assert(!std::is_move_constructible_v<ConditionVar>, "ConditionVar must not be move-constructible");
    static_assert(!std::is_move_assignable_v<ConditionVar>, "ConditionVar must not be move-assignable");
    cr_assert(true);
}

Test(condvar_notify, notify_one_wakes_waiter_with_predicate, .timeout = 5)
{
    Mutex m;
    ConditionVar cv;
    bool ready = false;
    std::atomic<bool> woken{false};

    std::thread waiter([&] {
        std::unique_lock<Mutex> lk(m);
        cv.wait(lk, [&] { return ready; });
        woken.store(true);
    });

    std::this_thread::sleep_for(50ms);
    {
        std::lock_guard<Mutex> lk(m);
        ready = true;
    }
    cv.notify_one();
    waiter.join();

    cr_assert(woken.load(), "waiter must observe predicate true after notify_one");
}

Test(condvar_notify, predicate_filters_spurious_wakeups, .timeout = 5)
{
    Mutex m;
    ConditionVar cv;
    bool ready = false;
    std::atomic<bool> woken{false};

    std::thread waiter([&] {
        std::unique_lock<Mutex> lk(m);
        cv.wait(lk, [&] { return ready; });
        woken.store(true);
    });

    std::this_thread::sleep_for(50ms);
    cv.notify_one();
    std::this_thread::sleep_for(50ms);

    cr_assert_not(woken.load(), "predicate must keep the waiter blocked when condition is still false");

    {
        std::lock_guard<Mutex> lk(m);
        ready = true;
    }
    cv.notify_one();
    waiter.join();

    cr_assert(woken.load(), "waiter must finish once the predicate becomes true");
}

Test(condvar_notify, notify_all_wakes_every_waiter, .timeout = 5)
{
    constexpr int waiter_count = 4;
    Mutex m;
    ConditionVar cv;
    bool ready = false;
    std::atomic<int> awake_count{0};

    std::vector<std::thread> waiters;
    waiters.reserve(waiter_count);
    for (int i = 0; i < waiter_count; ++i) {
        waiters.emplace_back([&] {
            std::unique_lock<Mutex> lk(m);
            cv.wait(lk, [&] { return ready; });
            awake_count.fetch_add(1);
        });
    }

    std::this_thread::sleep_for(50ms);
    {
        std::lock_guard<Mutex> lk(m);
        ready = true;
    }
    cv.notify_all();
    for (auto &t : waiters)
        t.join();

    cr_assert_eq(awake_count.load(), waiter_count,
        "notify_all must wake every waiter — got %d, expected %d",
        awake_count.load(), waiter_count);
}

Test(condvar_wait_for, times_out_when_no_notify, .timeout = 5)
{
    Mutex m;
    ConditionVar cv;
    std::unique_lock<Mutex> lk(m);

    const auto start = std::chrono::steady_clock::now();
    const auto status = cv.wait_for(lk, 80ms);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    cr_assert_eq(status, std::cv_status::timeout, "wait_for must report timeout when no notify arrived");
    cr_assert_geq(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 50,
        "wait_for must actually block for the requested duration");
}

Test(condvar_wait_for, predicate_overload_returns_false_on_timeout, .timeout = 5)
{
    Mutex m;
    ConditionVar cv;
    std::unique_lock<Mutex> lk(m);

    const bool predicate_satisfied = cv.wait_for(lk, 80ms, [] { return false; });

    cr_assert_not(predicate_satisfied, "wait_for(pred) must return false when timeout elapses with pred still false");
}

Test(condvar_wait_for, predicate_overload_returns_true_when_notified, .timeout = 5)
{
    Mutex m;
    ConditionVar cv;
    bool ready = false;

    std::thread notifier([&] {
        std::this_thread::sleep_for(50ms);
        {
            std::lock_guard<Mutex> lk(m);
            ready = true;
        }
        cv.notify_one();
    });

    std::unique_lock<Mutex> lk(m);
    const bool predicate_satisfied = cv.wait_for(lk, 5s, [&] { return ready; });
    lk.unlock();
    notifier.join();

    cr_assert(predicate_satisfied, "wait_for(pred) must return true once the predicate flips before timeout");
}

Test(condvar_interop, works_with_std_mutex_lock, .timeout = 5)
{
    std::mutex m;
    ConditionVar cv;
    bool ready = false;
    std::atomic<bool> woken{false};

    std::thread waiter([&] {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return ready; });
        woken.store(true);
    });

    std::this_thread::sleep_for(50ms);
    {
        std::lock_guard<std::mutex> lk(m);
        ready = true;
    }
    cv.notify_one();
    waiter.join();

    cr_assert(woken.load(), "ConditionVar must also accept std::unique_lock<std::mutex> thanks to condition_variable_any");
}

Test(condvar_pingpong, two_threads_alternate_via_notify, .timeout = 5)
{
    Mutex m;
    ConditionVar cv;
    int turn = 0;
    constexpr int turns_per_thread = 100;

    std::thread even([&] {
        std::unique_lock<Mutex> lk(m);
        for (int i = 0; i < turns_per_thread; ++i) {
            cv.wait(lk, [&] { return turn % 2 == 0; });
            ++turn;
            cv.notify_all();
        }
    });
    std::thread odd([&] {
        std::unique_lock<Mutex> lk(m);
        for (int i = 0; i < turns_per_thread; ++i) {
            cv.wait(lk, [&] { return turn % 2 == 1; });
            ++turn;
            cv.notify_all();
        }
    });

    even.join();
    odd.join();

    cr_assert_eq(turn, 2 * turns_per_thread,
        "ping-pong must advance turn exactly 2N times — got %d", turn);
}
