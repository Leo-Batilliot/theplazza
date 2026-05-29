#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "concurrency/ConditionVar.hpp"
#include "concurrency/Mutex.hpp"
#include "kitchen/ThreadPool.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

using plazza::KitchenError;
using plazza::concurrency::ConditionVar;
using plazza::concurrency::Mutex;
using plazza::kitchen::ThreadPool;
using namespace std::chrono_literals;

Test(threadpool_construction, zero_workers_is_rejected)
{
    cr_assert_throw(ThreadPool{0}, KitchenError);
}

Test(threadpool_construction, size_reports_worker_count)
{
    ThreadPool pool(4);

    cr_assert_eq(pool.size(), 4u);
    cr_assert_not(pool.isShutdown());
}

Test(threadpool_submit, single_task_executes, .timeout = 5)
{
    ThreadPool pool(1);
    std::atomic<bool> ran{false};

    pool.submit([&] { ran.store(true); });
    pool.shutdown();

    cr_assert(ran.load());
}

Test(threadpool_submit, task_runs_on_worker_not_caller, .timeout = 5)
{
    ThreadPool pool(1);
    std::atomic<std::thread::id> seen{};

    pool.submit([&] { seen.store(std::this_thread::get_id()); });
    pool.shutdown();

    cr_assert_neq(seen.load(), std::this_thread::get_id());
}

Test(threadpool_submit, empty_task_rejected)
{
    ThreadPool pool(1);

    cr_assert_throw(pool.submit({}), KitchenError);
}

Test(threadpool_submit, many_tasks_all_execute, .timeout = 5)
{
    ThreadPool pool(4);
    constexpr int total = 200;
    std::atomic<int> counter{0};

    for (int i = 0; i < total; ++i)
        pool.submit([&] { counter.fetch_add(1); });
    pool.shutdown();

    cr_assert_eq(counter.load(), total);
}

Test(threadpool_parallel, multiple_workers_run_concurrently, .timeout = 5)
{
    constexpr std::size_t workers = 4;
    ThreadPool pool(workers);
    std::atomic<int> inFlight{0};
    std::atomic<int> peak{0};

    for (std::size_t i = 0; i < workers; ++i) {
        pool.submit([&] {
            const int now = inFlight.fetch_add(1) + 1;
            int prev = peak.load();
            while (now > prev && !peak.compare_exchange_weak(prev, now)) {}
            std::this_thread::sleep_for(50ms);
            inFlight.fetch_sub(1);
        });
    }
    pool.shutdown();

    cr_assert_eq(peak.load(), static_cast<int>(workers));
}

Test(threadpool_parallel, distinct_thread_ids_used, .timeout = 5)
{
    constexpr std::size_t workers = 4;
    ThreadPool pool(workers);

    Mutex mtx;
    ConditionVar cv;
    std::set<std::thread::id> seen;
    int arrived = 0;
    bool release = false;

    for (std::size_t i = 0; i < workers; ++i) {
        pool.submit([&] {
            std::unique_lock<Mutex> lk(mtx);
            seen.insert(std::this_thread::get_id());
            ++arrived;
            cv.notify_all();
            cv.wait(lk, [&] { return release; });
        });
    }

    {
        std::unique_lock<Mutex> lk(mtx);
        cv.wait(lk, [&] { return arrived == static_cast<int>(workers); });
        release = true;
    }
    cv.notify_all();
    pool.shutdown();

    cr_assert_eq(seen.size(), workers);
}

Test(threadpool_order, single_worker_runs_tasks_in_fifo_order, .timeout = 5)
{
    ThreadPool pool(1);
    Mutex mtx;
    std::vector<int> sequence;

    constexpr int total = 50;
    for (int i = 0; i < total; ++i) {
        pool.submit([&, i] {
            std::lock_guard<Mutex> lk(mtx);
            sequence.push_back(i);
        });
    }
    pool.shutdown();

    cr_assert_eq(sequence.size(), static_cast<std::size_t>(total));
    for (int i = 0; i < total; ++i)
        cr_assert_eq(sequence[i], i);
}

Test(threadpool_shutdown, submit_after_shutdown_throws, .timeout = 5)
{
    ThreadPool pool(2);
    pool.shutdown();

    cr_assert(pool.isShutdown());
    cr_assert_throw(pool.submit([] {}), KitchenError);
}

Test(threadpool_shutdown, shutdown_drains_pending_queue, .timeout = 10)
{
    ThreadPool pool(1);
    std::atomic<int> done{0};

    Mutex mtx;
    ConditionVar cv;
    bool release = false;
    bool blockerStarted = false;

    pool.submit([&] {
        std::unique_lock<Mutex> lk(mtx);
        blockerStarted = true;
        cv.notify_all();
        cv.wait(lk, [&] { return release; });
        done.fetch_add(1);
    });

    {
        std::unique_lock<Mutex> lk(mtx);
        cv.wait(lk, [&] { return blockerStarted; });
    }

    constexpr int queued = 20;
    for (int i = 0; i < queued; ++i)
        pool.submit([&] { done.fetch_add(1); });

    cr_assert_eq(pool.pending(), static_cast<std::size_t>(queued));

    {
        std::lock_guard<Mutex> lk(mtx);
        release = true;
    }
    cv.notify_all();
    pool.shutdown();

    cr_assert_eq(done.load(), queued + 1);
}

Test(threadpool_shutdown, shutdown_now_drops_pending_queue, .timeout = 10)
{
    ThreadPool pool(1);
    std::atomic<int> done{0};

    Mutex mtx;
    ConditionVar cv;
    bool release = false;
    bool blockerStarted = false;

    pool.submit([&] {
        std::unique_lock<Mutex> lk(mtx);
        blockerStarted = true;
        cv.notify_all();
        cv.wait(lk, [&] { return release; });
        done.fetch_add(1);
    });

    {
        std::unique_lock<Mutex> lk(mtx);
        cv.wait(lk, [&] { return blockerStarted; });
    }

    constexpr int queued = 20;
    for (int i = 0; i < queued; ++i)
        pool.submit([&] { done.fetch_add(1); });

    {
        std::lock_guard<Mutex> lk(mtx);
        release = true;
    }
    cv.notify_all();
    pool.shutdownNow();

    cr_assert_eq(done.load(), 1);
    cr_assert_eq(pool.pending(), 0u);
}

Test(threadpool_shutdown, double_shutdown_is_idempotent, .timeout = 5)
{
    ThreadPool pool(2);
    pool.shutdown();
    pool.shutdown();

    cr_assert(pool.isShutdown());
}

Test(threadpool_destructor, destructor_joins_running_workers, .timeout = 5)
{
    std::atomic<int> done{0};
    {
        ThreadPool pool(4);
        for (int i = 0; i < 8; ++i)
            pool.submit([&] {
                std::this_thread::sleep_for(20ms);
                done.fetch_add(1);
            });
    }
    cr_assert_eq(done.load(), 8);
}

Test(threadpool_status, active_workers_reflects_running_tasks, .timeout = 5)
{
    ThreadPool pool(3);
    Mutex mtx;
    ConditionVar cv;
    int started = 0;
    bool release = false;

    for (int i = 0; i < 3; ++i) {
        pool.submit([&] {
            std::unique_lock<Mutex> lk(mtx);
            ++started;
            cv.notify_all();
            cv.wait(lk, [&] { return release; });
        });
    }

    {
        std::unique_lock<Mutex> lk(mtx);
        cv.wait(lk, [&] { return started == 3; });
    }
    cr_assert_eq(pool.activeWorkers(), 3u);

    {
        std::lock_guard<Mutex> lk(mtx);
        release = true;
    }
    cv.notify_all();
    pool.shutdown();

    cr_assert_eq(pool.activeWorkers(), 0u);
}

Test(threadpool_stress, thousand_tasks_across_four_workers, .timeout = 10)
{
    ThreadPool pool(4);
    constexpr int total = 1000;
    std::atomic<int> counter{0};

    for (int i = 0; i < total; ++i)
        pool.submit([&] { counter.fetch_add(1); });
    pool.shutdown();

    cr_assert_eq(counter.load(), total);
}

Test(threadpool_exception, task_exception_does_not_kill_worker, .timeout = 5)
{
    ThreadPool pool(1);
    std::atomic<int> ranAfter{0};

    pool.submit([] { throw std::runtime_error("boom"); });
    pool.submit([&] { ranAfter.fetch_add(1); });
    pool.submit([&] { ranAfter.fetch_add(1); });
    pool.shutdown();

    cr_assert_eq(ranAfter.load(), 2);
}
