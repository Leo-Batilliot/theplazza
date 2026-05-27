#include "concurrency/Thread.hpp"

namespace plazza::concurrency {

Thread::~Thread()
{
    if (_thread.joinable())
        _thread.join();
}

Thread &Thread::operator=(Thread &&other) noexcept
{
    if (this == &other)
        return *this;
    if (_thread.joinable())
        _thread.join();
    _thread = std::move(other._thread);
    return *this;
}

void Thread::join()
{
    _thread.join();
}

void Thread::detach()
{
    _thread.detach();
}

}
