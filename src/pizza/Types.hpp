#pragma once

#include <cstdint>

namespace plazza::pizza {

enum class PizzaType : std::uint32_t {
    Regina    = 1,
    Margarita = 2,
    Americana = 4,
    Fantasia  = 8,
};

enum class PizzaSize : std::uint32_t {
    S   = 1,
    M   = 2,
    L   = 4,
    XL  = 8,
    XXL = 16,
};

enum class Ingredient : std::uint32_t {
    Dough,
    Tomato,
    Gruyere,
    Ham,
    Mushrooms,
    Steak,
    Eggplant,
    GoatCheese,
    ChiefLove,
};

inline constexpr std::uint32_t ingredientCount = 9;

}
