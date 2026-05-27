#pragma once

#include "parser/Lexer.hpp"
#include "pizza/Types.hpp"

#include <string>
#include <vector>

namespace plazza::parser {

struct ParsedOrder {
    pizza::PizzaType type;
    pizza::PizzaSize size;
    int count;
};

class Parser {
public:
    explicit Parser(std::string source);

    Parser(const Parser &) = delete;
    Parser &operator=(const Parser &) = delete;
    Parser(Parser &&) noexcept = default;
    Parser &operator=(Parser &&) noexcept = default;
    ~Parser() = default;

    [[nodiscard]] std::vector<ParsedOrder> parse();

private:
    ParsedOrder parseOrder();
    pizza::PizzaType parseType();
    pizza::PizzaSize parseSize();
    int parseNumber();

    Lexer _lexer;
};

}
