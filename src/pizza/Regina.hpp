#pragma once

#include "pizza/Pizza.hpp"
#include "pizza/Types.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace plazza::pizza {

class Regina : public Pizza {
public:
    explicit Regina(PizzaSize size) noexcept;

    [[nodiscard]] PizzaType type() const noexcept override;
    [[nodiscard]] std::string name() const override;
    [[nodiscard]] std::vector<Ingredient> ingredients() const override;
    [[nodiscard]] std::chrono::milliseconds baseCookTime() const noexcept override;
};

}
