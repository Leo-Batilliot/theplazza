#include "pizza/Regina.hpp"

#include "pizza/PizzaFactory.hpp"

namespace plazza::pizza {

Regina::Regina(PizzaSize size) noexcept : Pizza(size) {}

PizzaType Regina::type() const noexcept
{
    return PizzaType::Regina;
}

std::string Regina::name() const
{
    return "regina";
}

std::vector<Ingredient> Regina::ingredients() const
{
    return {Ingredient::Dough, Ingredient::Tomato, Ingredient::Gruyere,
            Ingredient::Ham, Ingredient::Mushrooms};
}

std::chrono::milliseconds Regina::baseCookTime() const noexcept
{
    return std::chrono::milliseconds(2000);
}

namespace {
const PizzaRegistrar<Regina> _registrar{"regina", PizzaType::Regina};
}

}
