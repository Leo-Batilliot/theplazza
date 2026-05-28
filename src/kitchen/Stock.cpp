#include "kitchen/Stock.hpp"

#include "common/Errors.hpp"

#include <cstddef>
#include <mutex>
#include <string>

namespace plazza::kitchen {

namespace {

std::size_t indexOf(pizza::Ingredient ingredient) noexcept
{
    return static_cast<std::size_t>(ingredient);
}

}

Stock::Stock() noexcept : Stock(initialCount) {}

Stock::Stock(std::uint32_t initial) noexcept : _counts{}, _shutdown(false)
{
    for (auto &slot : _counts)
        slot = initial;
}

Stock::Counts Stock::aggregate(const std::vector<pizza::Ingredient> &ingredients)
{
    Counts required{};
    for (const auto ingredient : ingredients) {
        const auto idx = indexOf(ingredient);
        if (idx >= required.size())
            throw StockError("Stock: invalid ingredient index " + std::to_string(idx));
        ++required[idx];
    }
    return required;
}

bool Stock::hasEnoughLocked(const Counts &required) const noexcept
{
    for (std::size_t i = 0; i < required.size(); ++i) {
        if (_counts[i] < required[i])
            return false;
    }
    return true;
}

void Stock::consumeLocked(const Counts &required) noexcept
{
    for (std::size_t i = 0; i < required.size(); ++i)
        _counts[i] -= required[i];
}

bool Stock::tryConsume(const std::vector<pizza::Ingredient> &ingredients)
{
    const Counts required = aggregate(ingredients);
    std::unique_lock<concurrency::Mutex> lock(_mutex);
    if (_shutdown || !hasEnoughLocked(required))
        return false;
    consumeLocked(required);
    return true;
}

bool Stock::waitAndConsume(const std::vector<pizza::Ingredient> &ingredients)
{
    const Counts required = aggregate(ingredients);
    std::unique_lock<concurrency::Mutex> lock(_mutex);
    _condition.wait(lock, [&]() {
        return _shutdown || hasEnoughLocked(required);
    });
    if (_shutdown)
        return false;
    consumeLocked(required);
    return true;
}

void Stock::restock(pizza::Ingredient ingredient, std::uint32_t amount)
{
    if (amount == 0)
        return;
    const auto idx = indexOf(ingredient);
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        if (idx >= _counts.size())
            throw StockError("Stock::restock: invalid ingredient index " + std::to_string(idx));
        _counts[idx] += amount;
    }
    _condition.notify_all();
}

void Stock::restockAll(std::uint32_t amount)
{
    if (amount == 0)
        return;
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        for (auto &slot : _counts)
            slot += amount;
    }
    _condition.notify_all();
}

std::uint32_t Stock::available(pizza::Ingredient ingredient) const
{
    const auto idx = indexOf(ingredient);
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    if (idx >= _counts.size())
        throw StockError("Stock::available: invalid ingredient index " + std::to_string(idx));
    return _counts[idx];
}

std::array<std::uint32_t, pizza::ingredientCount> Stock::snapshot() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _counts;
}

void Stock::shutdown()
{
    {
        std::lock_guard<concurrency::Mutex> lock(_mutex);
        _shutdown = true;
    }
    _condition.notify_all();
}

bool Stock::isShutdown() const
{
    std::lock_guard<concurrency::Mutex> lock(_mutex);
    return _shutdown;
}

}
