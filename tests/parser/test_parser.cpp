#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "parser/Parser.hpp"
#include "pizza/Types.hpp"

#include <string>
#include <vector>

using plazza::parser::ParsedOrder;
using plazza::parser::Parser;
using plazza::pizza::PizzaSize;
using plazza::pizza::PizzaType;

static std::vector<ParsedOrder> parse(const std::string &source)
{
    Parser parser(source);
    return parser.parse();
}

Test(parser_empty, empty_input_yields_empty_vector)
{
    const auto orders = parse("");

    cr_assert_eq(orders.size(), 0u);
}

Test(parser_empty, whitespace_only_yields_empty_vector)
{
    const auto orders = parse("    \t\t  ");

    cr_assert_eq(orders.size(), 0u);
}

Test(parser_valid, single_order_regina_xxl_x2)
{
    const auto orders = parse("regina XXL x2");

    cr_assert_eq(orders.size(), 1u);
    cr_assert_eq(orders[0].type, PizzaType::Regina);
    cr_assert_eq(orders[0].size, PizzaSize::XXL);
    cr_assert_eq(orders[0].count, 2);
}

Test(parser_valid, single_order_margarita_l_x1)
{
    const auto orders = parse("margarita L x1");

    cr_assert_eq(orders.size(), 1u);
    cr_assert_eq(orders[0].type, PizzaType::Margarita);
    cr_assert_eq(orders[0].size, PizzaSize::L);
    cr_assert_eq(orders[0].count, 1);
}

Test(parser_valid, spec_multi_order_example)
{
    const auto orders = parse("regina XXL x2; fantasia M x3; margarita S x1");

    cr_assert_eq(orders.size(), 3u);
    cr_assert_eq(orders[0].type, PizzaType::Regina);
    cr_assert_eq(orders[0].size, PizzaSize::XXL);
    cr_assert_eq(orders[0].count, 2);
    cr_assert_eq(orders[1].type, PizzaType::Fantasia);
    cr_assert_eq(orders[1].size, PizzaSize::M);
    cr_assert_eq(orders[1].count, 3);
    cr_assert_eq(orders[2].type, PizzaType::Margarita);
    cr_assert_eq(orders[2].size, PizzaSize::S);
    cr_assert_eq(orders[2].count, 1);
}

Test(parser_valid, every_pizza_type_is_parseable)
{
    const auto orders = parse("regina M x1; americana M x1; fantasia M x1; margarita M x1");

    cr_assert_eq(orders.size(), 4u);
    cr_assert_eq(orders[0].type, PizzaType::Regina);
    cr_assert_eq(orders[1].type, PizzaType::Americana);
    cr_assert_eq(orders[2].type, PizzaType::Fantasia);
    cr_assert_eq(orders[3].type, PizzaType::Margarita);
}

Test(parser_valid, every_size_is_parseable)
{
    const auto orders = parse("regina S x1; regina M x1; regina L x1; regina XL x1; regina XXL x1");

    cr_assert_eq(orders.size(), 5u);
    cr_assert_eq(orders[0].size, PizzaSize::S);
    cr_assert_eq(orders[1].size, PizzaSize::M);
    cr_assert_eq(orders[2].size, PizzaSize::L);
    cr_assert_eq(orders[3].size, PizzaSize::XL);
    cr_assert_eq(orders[4].size, PizzaSize::XXL);
}

Test(parser_valid, type_is_case_insensitive)
{
    const auto orders = parse("REGINA M x1; Margarita M x1; fAnTaSiA M x1");

    cr_assert_eq(orders.size(), 3u);
    cr_assert_eq(orders[0].type, PizzaType::Regina);
    cr_assert_eq(orders[1].type, PizzaType::Margarita);
    cr_assert_eq(orders[2].type, PizzaType::Fantasia);
}

Test(parser_valid, size_is_case_insensitive)
{
    const auto orders = parse("regina xxl x1; regina Xl x1; regina m x1");

    cr_assert_eq(orders.size(), 3u);
    cr_assert_eq(orders[0].size, PizzaSize::XXL);
    cr_assert_eq(orders[1].size, PizzaSize::XL);
    cr_assert_eq(orders[2].size, PizzaSize::M);
}

Test(parser_valid, large_count)
{
    const auto orders = parse("margarita M x999");

    cr_assert_eq(orders.size(), 1u);
    cr_assert_eq(orders[0].count, 999);
}

Test(parser_valid, no_whitespace_around_semicolons)
{
    const auto orders = parse("regina M x1;fantasia L x2;americana S x3");

    cr_assert_eq(orders.size(), 3u);
    cr_assert_eq(orders[2].type, PizzaType::Americana);
    cr_assert_eq(orders[2].size, PizzaSize::S);
    cr_assert_eq(orders[2].count, 3);
}

Test(parser_invalid_spec, number_starts_with_zero_rejected)
{
    cr_assert_throw(parse("regina xxl x0"), plazza::ParseError);
}

Test(parser_invalid_spec, unknown_size_rejected)
{
    cr_assert_throw(parse("fantasia Z x1"), plazza::ParseError);
}

Test(parser_invalid_spec, unknown_type_rejected)
{
    cr_assert_throw(parse("unknownpizza M x1"), plazza::ParseError);
}

Test(parser_invalid_spec, empty_segment_double_semicolon_rejected)
{
    cr_assert_throw(parse("regina M x1;;"), plazza::ParseError);
}

Test(parser_invalid_structure, trailing_semicolon_rejected)
{
    cr_assert_throw(parse("regina M x1;"), plazza::ParseError);
}

Test(parser_invalid_structure, missing_size_and_number_rejected)
{
    cr_assert_throw(parse("regina"), plazza::ParseError);
}

Test(parser_invalid_structure, missing_number_rejected)
{
    cr_assert_throw(parse("regina M"), plazza::ParseError);
}

Test(parser_invalid_structure, wrong_token_order_rejected)
{
    cr_assert_throw(parse("x1 regina M"), plazza::ParseError);
}

Test(parser_invalid_structure, number_where_size_expected_rejected)
{
    cr_assert_throw(parse("regina x1 x2"), plazza::ParseError);
}

Test(parser_invalid_structure, two_orders_missing_separator_rejected)
{
    cr_assert_throw(parse("regina M x1 fantasia M x2"), plazza::ParseError);
}

Test(parser_invalid_structure, leading_semicolon_rejected)
{
    cr_assert_throw(parse(";regina M x1"), plazza::ParseError);
}

Test(parser_invalid_structure, second_order_invalid_keeps_throwing)
{
    cr_assert_throw(parse("regina M x1; unknownpizza M x1"), plazza::ParseError);
}

Test(parser_invalid_lex, stray_character_rejected)
{
    cr_assert_throw(parse("regina M x1 @"), plazza::ParseError);
}
