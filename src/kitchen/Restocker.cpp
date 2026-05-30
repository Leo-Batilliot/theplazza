#include "kitchen/Restocker.hpp"

#include "common/Errors.hpp"
#include "kitchen/Stock.hpp"

#include <mutex>

namespace plazza::kitchen {

Restocker::Restocker(Stock &stock, std::chrono::milliseconds period, std::uint32_t amount)
    : _stock(stock), _period(period), _amount(amount), _running(false), _stopRequested(false)
{
    if (period.count() <= 0)
        throw KitchenError("Restocker: period must be positive");
}

Restocker::~Restocker()
{
    stop();
}

void Restocker::start()
{
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        if (_running)
            throw KitchenError("Restocker::start: already running");
        _stopRequested = false;
        _running = true;
    }
    _thread = concurrency::Thread([this] { loop(); });
}

void Restocker::stop()
{
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        if (!_running && !_thread.joinable())
            return;
        _stopRequested = true;
    }
    _condition.notify_all();
    if (_thread.joinable())
        _thread.join();
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    _running = false;
}

bool Restocker::running() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _running && !_stopRequested;
}

void Restocker::loop()
{
    while (true) {
        {
            std::unique_lock<concurrency::Mutex> lock(_mutex);
            if (_condition.wait_for(lock, _period, [this] { return _stopRequested; }))
                return;
        }
        _stock.restockAll(_amount);
    }
}

}
