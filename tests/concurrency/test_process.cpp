#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "concurrency/Process.hpp"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <thread>
#include <type_traits>
#include <utility>

using plazza::ProcessError;
using plazza::concurrency::ExitStatus;
using plazza::concurrency::Process;
using namespace std::chrono_literals;

Test(process_traits, is_non_copyable_and_movable)
{
    static_assert(!std::is_copy_constructible_v<Process>, "Process must not be copy-constructible");
    static_assert(!std::is_copy_assignable_v<Process>, "Process must not be copy-assignable");
    static_assert(std::is_move_constructible_v<Process>, "Process must be move-constructible");
    static_assert(std::is_move_assignable_v<Process>, "Process must be move-assignable");
    cr_assert(true);
}

Test(process_basic, default_constructed_is_not_joinable)
{
    Process p;
    cr_assert_not(p.joinable(), "default-constructed Process must not be joinable");
    cr_assert_eq(p.pid(), 0, "default-constructed Process must have pid 0");
}

Test(process_basic, constructor_forks_and_assigns_pid, .timeout = 5)
{
    Process p([] { return 0; });
    cr_assert(p.joinable(), "Process built from an entry must be joinable");
    cr_assert_gt(p.pid(), 0, "child pid must be positive in the parent");
    auto status = p.wait();
    cr_assert(status.exited);
    cr_assert_eq(status.code, 0);
}

Test(process_basic, exit_code_is_propagated, .timeout = 5)
{
    Process p([] { return 42; });
    auto status = p.wait();
    cr_assert(status.exited, "child must report a normal exit");
    cr_assert_not(status.signaled, "child must not appear signal-terminated");
    cr_assert_eq(status.code, 42, "child's returned int must surface as ExitStatus::code");
}

Test(process_basic, non_zero_exit_code_is_preserved, .timeout = 5)
{
    Process p([] { return 7; });
    auto status = p.wait();
    cr_assert(status.exited);
    cr_assert_eq(status.code, 7);
}

Test(process_basic, throwing_entry_exits_with_failure_code, .timeout = 5)
{
    Process p([]() -> int { throw plazza::Error("boom"); });
    auto status = p.wait();
    cr_assert(status.exited, "uncaught exception in child must still exit cleanly");
    cr_assert_eq(status.code, 1, "thrown exception in entry must surface as exit code 1");
}

Test(process_basic, null_entry_throws_process_error)
{
    bool threw = false;
    try {
        Process p{std::function<int()>{}};
    } catch (const ProcessError &) {
        threw = true;
    }
    cr_assert(threw, "null entry function must throw ProcessError");
}

Test(process_wait, wait_blocks_until_child_exits, .timeout = 5)
{
    Process p([] {
        ::usleep(50 * 1000);
        return 3;
    });
    auto start = std::chrono::steady_clock::now();
    auto status = p.wait();
    auto elapsed = std::chrono::steady_clock::now() - start;

    cr_assert(status.exited);
    cr_assert_eq(status.code, 3);
    cr_assert(elapsed >= 40ms, "wait() must block at least until the child has slept");
    cr_assert_not(p.joinable(), "Process must no longer be joinable after a successful wait()");
}

Test(process_wait, wait_on_non_joinable_throws)
{
    Process p;
    bool threw = false;
    try {
        (void)p.wait();
    } catch (const ProcessError &) {
        threw = true;
    }
    cr_assert(threw, "wait() on a default Process must throw ProcessError");
}

Test(process_try_wait, returns_empty_while_child_runs, .timeout = 5)
{
    Process p([] {
        ::usleep(100 * 1000);
        return 0;
    });

    auto first = p.try_wait();
    cr_assert(!first.has_value(), "try_wait() must report nullopt while child is alive");
    cr_assert(p.joinable(), "try_wait() must not clear joinable state when child still runs");

    auto status = p.wait();
    cr_assert(status.exited);
    cr_assert_eq(status.code, 0);
}

Test(process_try_wait, returns_status_after_exit, .timeout = 5)
{
    Process p([] { return 11; });

    std::optional<ExitStatus> probed;
    for (int i = 0; i < 100; ++i) {
        probed = p.try_wait();
        if (probed.has_value())
            break;
        std::this_thread::sleep_for(10ms);
    }
    cr_assert(probed.has_value(), "try_wait() must eventually observe child exit");
    cr_assert(probed->exited);
    cr_assert_eq(probed->code, 11);
    cr_assert_not(p.joinable(), "Process must be cleared after try_wait() observes exit");
}

Test(process_try_wait, on_non_joinable_throws)
{
    Process p;
    bool threw = false;
    try {
        (void)p.try_wait();
    } catch (const ProcessError &) {
        threw = true;
    }
    cr_assert(threw);
}

Test(process_kill, sigterm_terminates_child, .timeout = 5)
{
    Process p([] {
        for (;;)
            ::pause();
        return 0;
    });

    p.kill(SIGTERM);
    auto status = p.wait();

    cr_assert(status.signaled, "child killed by SIGTERM must report signaled=true");
    cr_assert_eq(status.signal, SIGTERM, "termination signal must be SIGTERM");
    cr_assert_not(status.exited, "signaled child must not also be marked exited");
}

Test(process_kill, sigkill_is_lethal, .timeout = 5)
{
    Process p([] {
        for (;;)
            ::pause();
        return 0;
    });

    p.kill(SIGKILL);
    auto status = p.wait();
    cr_assert(status.signaled);
    cr_assert_eq(status.signal, SIGKILL);
}

Test(process_kill, on_non_joinable_throws)
{
    Process p;
    bool threw = false;
    try {
        p.kill(SIGTERM);
    } catch (const ProcessError &) {
        threw = true;
    }
    cr_assert(threw);
}

Test(process_detach, detach_clears_joinable_state, .timeout = 5)
{
    Process p([] { return 0; });
    pid_t child = p.pid();
    p.detach();

    cr_assert_not(p.joinable(), "detach() must clear joinable state");
    cr_assert_eq(p.pid(), 0, "detach() must clear the stored pid");

    int status = 0;
    pid_t reaped = 0;
    for (int i = 0; i < 100; ++i) {
        reaped = ::waitpid(child, &status, WNOHANG);
        if (reaped == child)
            break;
        std::this_thread::sleep_for(10ms);
    }
    cr_assert_eq(reaped, child, "detached child still exits and can be reaped manually");
}

Test(process_move, move_construction_transfers_ownership, .timeout = 5)
{
    Process source([] {
        ::usleep(20 * 1000);
        return 5;
    });
    pid_t source_pid = source.pid();

    Process sink(std::move(source));
    cr_assert_not(source.joinable(), "moved-from Process must not be joinable");
    cr_assert(sink.joinable());
    cr_assert_eq(sink.pid(), source_pid, "sink must inherit the original child pid");

    auto status = sink.wait();
    cr_assert(status.exited);
    cr_assert_eq(status.code, 5);
}

Test(process_move, move_assignment_waits_existing_child, .timeout = 10)
{
    Process lhs([] {
        ::usleep(20 * 1000);
        return 1;
    });
    Process rhs([] {
        ::usleep(200 * 1000);
        return 2;
    });
    pid_t lhs_pid = lhs.pid();
    pid_t rhs_pid = rhs.pid();

    lhs = std::move(rhs);

    cr_assert_not(rhs.joinable(), "rhs must be empty after being moved from");
    cr_assert(lhs.joinable(), "lhs must now own rhs's child");
    cr_assert_eq(lhs.pid(), rhs_pid, "lhs must hold the rhs pid");

    int status = 0;
    cr_assert_eq(::waitpid(lhs_pid, &status, WNOHANG), -1,
        "old lhs child must already have been reaped by move-assignment");

    auto final_status = lhs.wait();
    cr_assert(final_status.exited);
    cr_assert_eq(final_status.code, 2);
}

Test(process_move, self_move_assignment_is_safe, .timeout = 5)
{
    Process p([] { return 9; });
    Process &ref = p;
    p = std::move(ref);

    cr_assert(p.joinable(), "self move-assignment must not destroy the child");
    auto status = p.wait();
    cr_assert(status.exited);
    cr_assert_eq(status.code, 9);
}

Test(process_raii, destructor_reaps_running_child, .timeout = 5)
{
    pid_t child = 0;
    {
        Process p([] {
            ::usleep(20 * 1000);
            return 0;
        });
        child = p.pid();
        cr_assert_gt(child, 0);
    }
    int status = 0;
    cr_assert_eq(::waitpid(child, &status, WNOHANG), -1,
        "destructor must have reaped the child — no zombie should remain");
}

Test(process_isolation, child_state_does_not_leak_to_parent, .timeout = 5)
{
    int parent_local = 7;
    Process p([&parent_local] {
        parent_local = 999;
        return 0;
    });
    auto status = p.wait();

    cr_assert(status.exited);
    cr_assert_eq(parent_local, 7, "child writes to parent locals must not affect the parent (fork copy)");
}
