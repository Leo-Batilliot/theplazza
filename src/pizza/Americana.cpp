#include "pizza/Americana.hpp"

#include "pizza/PizzaFactory.hpp"

namespace plazza::pizza {

Americana::Americana(PizzaSize size) noexcept : Pizza(size) {}

PizzaType Americana::type() const noexcept
{
    return PizzaType::Americana;
}

std::string Americana::name() const
{
    return "americana";
}

std::vector<Ingredient> Americana::ingredients() const
{
    return {Ingredient::Dough, Ingredient::Tomato, Ingredient::Gruyere,
            Ingredient::Steak};
}

std::chrono::milliseconds Americana::baseCookTime() const noexcept
{
    return std::chrono::milliseconds(2000);
}

namespace {
const PizzaRegistrar<Americana> _registrar{"americana", PizzaType::Americana};
}

}
