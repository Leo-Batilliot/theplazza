#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "pizza/Americana.hpp"
#include "pizza/Fantasia.hpp"
#include "pizza/Margarita.hpp"
#include "pizza/PizzaFactory.hpp"
#include "pizza/Regina.hpp"
#include "pizza/Types.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using plazza::pizza::Americana;
using plazza::pizza::Fantasia;
using plazza::pizza::Ingredient;
using plazza::pizza::Margarita;
using plazza::pizza::Pizza;
using plazza::pizza::PizzaFactory;
using plazza::pizza::PizzaSize;
using plazza::pizza::PizzaType;
using plazza::pizza::Regina;

static bool hasIngredient(const std::vector<Ingredient> &list, Ingredient ing)
{
    return std::find(list.begin(), list.end(), ing) != list.end();
}

Test(factory_registry, four_mandatory_pizzas_registered)
{
    const auto &factory = PizzaFactory::instance();

    cr_assert(factory.isRegistered(PizzaType::Margarita));
    cr_assert(factory.isRegistered(PizzaType::Regina));
    cr_assert(factory.isRegistered(PizzaType::Americana));
    cr_assert(factory.isRegistered(PizzaType::Fantasia));
    cr_assert_geq(factory.size(), 4u);
}

Test(factory_registry, names_round_trip_with_types)
{
    const auto &factory = PizzaFactory::instance();

    cr_assert_eq(factory.typeForName("margarita").value(), PizzaType::Margarita);
    cr_assert_eq(factory.typeForName("regina").value(),    PizzaType::Regina);
    cr_assert_eq(factory.typeForName("americana").value(), PizzaType::Americana);
    cr_assert_eq(factory.typeForName("fantasia").value(),  PizzaType::Fantasia);

    cr_assert_str_eq(factory.nameForType(PizzaType::Margarita).value().c_str(), "margarita");
    cr_assert_str_eq(factory.nameForType(PizzaType::Regina).value().c_str(),    "regina");
    cr_assert_str_eq(factory.nameForType(PizzaType::Americana).value().c_str(), "americana");
    cr_assert_str_eq(factory.nameForType(PizzaType::Fantasia).value().c_str(),  "fantasia");
}

Test(factory_registry, unknown_name_returns_nullopt)
{
    const auto &factory = PizzaFactory::instance();

    cr_assert_not(factory.typeForName("calzone").has_value());
    cr_assert_not(factory.isRegisteredName("calzone"));
}

Test(factory_create, by_type_produces_concrete_pizza)
{
    const auto &factory = PizzaFactory::instance();

    auto pizza = factory.create(PizzaType::Regina, PizzaSize::L);

    cr_assert_not_null(pizza.get());
    cr_assert_eq(pizza->type(), PizzaType::Regina);
    cr_assert_eq(pizza->size(), PizzaSize::L);
    cr_assert_str_eq(pizza->name().c_str(), "regina");
}

Test(factory_create, by_name_produces_concrete_pizza)
{
    const auto &factory = PizzaFactory::instance();

    auto pizza = factory.createByName("americana", PizzaSize::XXL);

    cr_assert_not_null(pizza.get());
    cr_assert_eq(pizza->type(), PizzaType::Americana);
    cr_assert_eq(pizza->size(), PizzaSize::XXL);
}

Test(factory_create, unknown_type_throws)
{
    const auto &factory = PizzaFactory::instance();

    cr_assert_throw((void)factory.create(static_cast<PizzaType>(9999), PizzaSize::M),
        plazza::Error);
}

Test(factory_create, unknown_name_throws)
{
    const auto &factory = PizzaFactory::instance();

    cr_assert_throw((void)factory.createByName("calzone", PizzaSize::M),
        plazza::Error);
}

Test(pizza_recipes, margarita_recipe_and_cook_time)
{
    Margarita p(PizzaSize::M);

    cr_assert_eq(p.type(), PizzaType::Margarita);
    cr_assert_eq(p.ingredients().size(), 3u);
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Dough));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Tomato));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Gruyere));
    cr_assert_eq(p.baseCookTime().count(), 1000);
}

Test(pizza_recipes, regina_recipe_and_cook_time)
{
    Regina p(PizzaSize::M);

    cr_assert_eq(p.type(), PizzaType::Regina);
    cr_assert_eq(p.ingredients().size(), 5u);
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Dough));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Tomato));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Gruyere));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Ham));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Mushrooms));
    cr_assert_eq(p.baseCookTime().count(), 2000);
}

Test(pizza_recipes, americana_recipe_and_cook_time)
{
    Americana p(PizzaSize::M);

    cr_assert_eq(p.type(), PizzaType::Americana);
    cr_assert_eq(p.ingredients().size(), 4u);
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Dough));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Tomato));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Gruyere));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Steak));
    cr_assert_eq(p.baseCookTime().count(), 2000);
}

Test(pizza_recipes, fantasia_recipe_and_cook_time)
{
    Fantasia p(PizzaSize::M);

    cr_assert_eq(p.type(), PizzaType::Fantasia);
    cr_assert_eq(p.ingredients().size(), 5u);
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Dough));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Tomato));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::Eggplant));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::GoatCheese));
    cr_assert(hasIngredient(p.ingredients(), Ingredient::ChiefLove));
    cr_assert_eq(p.baseCookTime().count(), 4000);
}

Test(pizza_set_size, size_can_be_changed_after_construction)
{
    auto pizza = PizzaFactory::instance().create(PizzaType::Margarita, PizzaSize::S);

    cr_assert_eq(pizza->size(), PizzaSize::S);
    pizza->setSize(PizzaSize::XXL);
    cr_assert_eq(pizza->size(), PizzaSize::XXL);
}
