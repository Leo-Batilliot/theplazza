#include "kitchen/Cook.hpp"

#include "common/Errors.hpp"
#include "kitchen/Stock.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

namespace plazza::kitchen {

Cook::Cook(Stock &stock, double timeMultiplier) noexcept
    : _stock(stock), _timeMultiplier(timeMultiplier <= 0.0 ? defaultTimeMultiplier : timeMultiplier)
{
}

std::chrono::milliseconds Cook::cookDuration(const pizza::Pizza &pizza) const noexcept
{
    const auto base = pizza.baseCookTime().count();
    const auto scaled = static_cast<long long>(static_cast<double>(base) * _timeMultiplier);
    return std::chrono::milliseconds(std::max<long long>(scaled, 0));
}

bool Cook::cook(const pizza::Pizza &pizza) const
{
    if (!_stock.waitAndConsume(pizza.ingredients()))
        return false;
    const auto duration = cookDuration(pizza);
    if (duration.count() > 0)
        std::this_thread::sleep_for(duration);
    return true;
}

}
