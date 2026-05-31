#include "reception/ChildReaper.hpp"

#include "common/Errors.hpp"

#include <sys/wait.h>

#include <cerrno>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace plazza::reception {

namespace {

concurrency::ExitStatus decode(int raw) noexcept
{
    concurrency::ExitStatus status;
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

ChildReaper::ChildReaper(OnExit onExit, std::chrono::milliseconds pollInterval)
    : _onExit(std::move(onExit)), _pollInterval(pollInterval),
      _running(false), _stopRequested(false)
{
    if (pollInterval.count() <= 0)
        throw Error("ChildReaper: poll interval must be positive");
}

ChildReaper::~ChildReaper()
{
    stop();
}

void ChildReaper::track(pid_t pid)
{
    if (pid <= 0)
        return;
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    _tracked[pid] = true;
}

void ChildReaper::untrack(pid_t pid)
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    _tracked.erase(pid);
}

void ChildReaper::start()
{
    if (_running.exchange(true))
        return;
    _stopRequested.store(false);
    _thread = concurrency::Thread([this] { loop(); });
}

void ChildReaper::stop()
{
    _stopRequested.store(true);
    if (_thread.joinable())
        _thread.join();
    _running.store(false);
}

void ChildReaper::loop()
{
    while (!_stopRequested.load()) {
        pollOnce();
        std::this_thread::sleep_for(_pollInterval);
    }
    pollOnce();
}

void ChildReaper::pollOnce()
{
    std::vector<pid_t> snapshot;
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        snapshot.reserve(_tracked.size());
        for (const auto &[pid, _] : _tracked)
            snapshot.push_back(pid);
    }
    for (pid_t pid : snapshot) {
        int raw = 0;
        const pid_t r = ::waitpid(pid, &raw, WNOHANG);
        if (r == 0)
            continue;
        if (r < 0) {
            if (errno == ECHILD) {
                std::lock_guard<concurrency::Mutex> lock(_mutex);
                _tracked.erase(pid);
            }
            continue;
        }
        {
            std::lock_guard<concurrency::Mutex> lock(_mutex);
            _tracked.erase(pid);
        }
        if (_onExit)
            _onExit(pid, decode(raw));
    }
}

}
