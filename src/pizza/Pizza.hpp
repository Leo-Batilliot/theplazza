#pragma once

#include "pizza/Types.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace plazza::pizza {

class Pizza {
public:
    explicit Pizza(PizzaSize size) noexcept;
    virtual ~Pizza() = default;

    Pizza(const Pizza &) = default;
    Pizza &operator=(const Pizza &) = default;
    Pizza(Pizza &&) noexcept = default;
    Pizza &operator=(Pizza &&) noexcept = default;

    [[nodiscard]] PizzaSize size() const noexcept { return _size; }
    void setSize(PizzaSize size) noexcept { _size = size; }

    [[nodiscard]] virtual PizzaType type() const noexcept = 0;
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual std::vector<Ingredient> ingredients() const = 0;
    [[nodiscard]] virtual std::chrono::milliseconds baseCookTime() const noexcept = 0;

protected:
    PizzaSize _size;
};

}
