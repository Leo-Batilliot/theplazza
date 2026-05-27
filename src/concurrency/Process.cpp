#include "concurrency/Process.hpp"

#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <utility>

#include "common/Errors.hpp"

namespace plazza::concurrency {

namespace {

ExitStatus decode(int raw)
{
    ExitStatus status;
    if (WIFEXITED(raw)) {
        status.exited = true;
        status.code = WEXITSTATUS(raw);
    } else if (WIFSIGNALED(raw)) {
        status.signaled = true;
        status.signal = WTERMSIG(raw);
    }
    return status;
}

}

Process::Process(Entry entry)
{
    if (!entry)
        throw ProcessError("Process: null entry function");

    const pid_t forked = ::fork();
    if (forked < 0)
        throw ProcessError(std::string("Process: fork failed: ") + std::strerror(errno));

    if (forked == 0) {
        int code = 1;
        try {
            code = entry();
        } catch (const std::exception &) {
            code = 1;
        } catch (...) {
            code = 1;
        }
        std::_Exit(code);
    }
    _pid = forked;
}

Process::~Process()
{
    if (_pid > 0) {
        int status = 0;
        ::waitpid(_pid, &status, 0);
        _pid = 0;
    }
}

Process::Process(Process &&other) noexcept : _pid(other._pid)
{
    other._pid = 0;
}

Process &Process::operator=(Process &&other) noexcept
{
    if (this == &other)
        return *this;
    if (_pid > 0) {
        int status = 0;
        ::waitpid(_pid, &status, 0);
    }
    _pid = other._pid;
    other._pid = 0;
    return *this;
}

ExitStatus Process::wait()
{
    if (_pid <= 0)
        throw ProcessError("Process::wait: no child to wait for");

    int raw = 0;
    pid_t result = 0;
    do {
        result = ::waitpid(_pid, &raw, 0);
    } while (result < 0 && errno == EINTR);

    if (result < 0)
        throw ProcessError(std::string("Process::wait: waitpid failed: ") + std::strerror(errno));

    _pid = 0;
    return decode(raw);
}

std::optional<ExitStatus> Process::try_wait()
{
    if (_pid <= 0)
        throw ProcessError("Process::try_wait: no child to wait for");

    int raw = 0;
    const pid_t result = ::waitpid(_pid, &raw, WNOHANG);
    if (result == 0)
        return std::nullopt;
    if (result < 0)
        throw ProcessError(std::string("Process::try_wait: waitpid failed: ") + std::strerror(errno));

    _pid = 0;
    return decode(raw);
}

void Process::kill(int signal)
{
    if (_pid <= 0)
        throw ProcessError("Process::kill: no child to signal");
    if (::kill(_pid, signal) < 0)
        throw ProcessError(std::string("Process::kill: kill failed: ") + std::strerror(errno));
}

void Process::detach() noexcept
{
    _pid = 0;
}

}
