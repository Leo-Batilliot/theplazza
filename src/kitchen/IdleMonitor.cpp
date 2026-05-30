#include "kitchen/IdleMonitor.hpp"

#include "common/Errors.hpp"

#include <mutex>

namespace plazza::kitchen {

IdleMonitor::IdleMonitor(duration threshold)
    : _threshold(threshold), _busy(0), _lastActivity(clock::now())
{
    if (threshold.count() <= 0)
        throw KitchenError("IdleMonitor: threshold must be positive");
}

void IdleMonitor::notifyActivity()
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    _lastActivity = clock::now();
}

void IdleMonitor::enter()
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    ++_busy;
    _lastActivity = clock::now();
}

void IdleMonitor::leave()
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    if (_busy == 0)
        throw KitchenError("IdleMonitor::leave: counter underflow");
    --_busy;
    if (_busy == 0)
        _lastActivity = clock::now();
}

bool IdleMonitor::isBusy() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _busy > 0;
}

std::size_t IdleMonitor::busyCount() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _busy;
}

IdleMonitor::duration IdleMonitor::idleDurationLocked() const
{
    if (_busy > 0)
        return duration::zero();
    return std::chrono::duration_cast<duration>(clock::now() - _lastActivity);
}

IdleMonitor::duration IdleMonitor::idleDuration() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return idleDurationLocked();
}

bool IdleMonitor::hasExpired() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    if (_busy > 0)
        return false;
    return idleDurationLocked() >= _threshold;
}

}
