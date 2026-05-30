#pragma once

#include "pizza/Pizza.hpp"

#include <chrono>

namespace plazza::kitchen {

class Stock;

class Cook {
public:
    static constexpr double defaultTimeMultiplier = 1.0;

    Cook(Stock &stock, double timeMultiplier) noexcept;
    ~Cook() = default;

    Cook(const Cook &) = delete;
    Cook &operator=(const Cook &) = delete;
    Cook(Cook &&) = delete;
    Cook &operator=(Cook &&) = delete;

    [[nodiscard]] bool cook(const pizza::Pizza &pizza) const;

    [[nodiscard]] double timeMultiplier() const noexcept { return _timeMultiplier; }
    [[nodiscard]] std::chrono::milliseconds cookDuration(const pizza::Pizza &pizza) const noexcept;

private:
    Stock &_stock;
    double _timeMultiplier;
};

}
