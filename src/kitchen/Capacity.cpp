#include "kitchen/Capacity.hpp"

#include "common/Errors.hpp"

#include <mutex>

namespace plazza::kitchen {

Capacity::Capacity(std::size_t maxConcurrent) : _max(maxConcurrent), _inUse(0)
{
    if (maxConcurrent == 0)
        throw KitchenError("Capacity: max must be > 0");
}

bool Capacity::tryAcquire()
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    if (_inUse >= _max)
        return false;
    ++_inUse;
    return true;
}

void Capacity::release()
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    if (_inUse == 0)
        throw KitchenError("Capacity::release: counter underflow");
    --_inUse;
}

std::size_t Capacity::inUse() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _inUse;
}

std::size_t Capacity::free() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _max - _inUse;
}

bool Capacity::full() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _inUse >= _max;
}

}
