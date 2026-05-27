#include "pizza/Fantasia.hpp"

#include "pizza/PizzaFactory.hpp"

namespace plazza::pizza {

Fantasia::Fantasia(PizzaSize size) noexcept : Pizza(size) {}

PizzaType Fantasia::type() const noexcept
{
    return PizzaType::Fantasia;
}

std::string Fantasia::name() const
{
    return "fantasia";
}

std::vector<Ingredient> Fantasia::ingredients() const
{
    return {Ingredient::Dough, Ingredient::Tomato, Ingredient::Eggplant,
            Ingredient::GoatCheese, Ingredient::ChiefLove};
}

std::chrono::milliseconds Fantasia::baseCookTime() const noexcept
{
    return std::chrono::milliseconds(4000);
}

namespace {
const PizzaRegistrar<Fantasia> _registrar{"fantasia", PizzaType::Fantasia};
}

}
