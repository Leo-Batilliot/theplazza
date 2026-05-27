#include "pizza/Margarita.hpp"

#include "pizza/PizzaFactory.hpp"

namespace plazza::pizza {

Margarita::Margarita(PizzaSize size) noexcept : Pizza(size) {}

PizzaType Margarita::type() const noexcept
{
    return PizzaType::Margarita;
}

std::string Margarita::name() const
{
    return "margarita";
}

std::vector<Ingredient> Margarita::ingredients() const
{
    return {Ingredient::Dough, Ingredient::Tomato, Ingredient::Gruyere};
}

std::chrono::milliseconds Margarita::baseCookTime() const noexcept
{
    return std::chrono::milliseconds(1000);
}

namespace {
const PizzaRegistrar<Margarita> _registrar{"margarita", PizzaType::Margarita};
}

}
