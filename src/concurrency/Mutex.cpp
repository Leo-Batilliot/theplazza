#include "concurrency/Mutex.hpp"

namespace plazza::concurrency {

void Mutex::lock()
{
    _mutex.lock();
}

void Mutex::unlock()
{
    _mutex.unlock();
}

bool Mutex::try_lock()
{
    return _mutex.try_lock();
}

}
