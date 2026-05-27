#include "parser/Parser.hpp"

#include "common/Errors.hpp"
#include "pizza/PizzaFactory.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace plazza::parser {

namespace {

std::string toLower(const std::string &input)
{
    std::string out(input.size(), '\0');
    std::transform(input.begin(), input.end(), out.begin(),
        [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
    return out;
}

std::string positionSuffix(const Token &token)
{
    return " at position " + std::to_string(token.position);
}

}

Parser::Parser(std::string source) : _lexer(std::move(source)) {}

std::vector<ParsedOrder> Parser::parse()
{
    std::vector<ParsedOrder> orders;

    if (_lexer.eof())
        return orders;

    orders.push_back(parseOrder());
    while (_lexer.peek().kind == TokenKind::Semicolon) {
        _lexer.next();
        orders.push_back(parseOrder());
    }
    if (!_lexer.eof()) {
        const Token trailing = _lexer.next();
        throw ParseError("Parser: unexpected token '" + trailing.lexeme + "'" +
            positionSuffix(trailing));
    }
    return orders;
}

ParsedOrder Parser::parseOrder()
{
    const pizza::PizzaType type = parseType();
    const pizza::PizzaSize size = parseSize();
    const int count = parseNumber();
    return ParsedOrder{type, size, count};
}

pizza::PizzaType Parser::parseType()
{
    const Token token = _lexer.next();
    if (token.kind != TokenKind::Ident)
        throw ParseError("Parser: expected pizza TYPE, got " +
            std::string(tokenKindName(token.kind)) + " '" + token.lexeme + "'" +
            positionSuffix(token));

    const std::string canonical = toLower(token.lexeme);
    const auto resolved = pizza::PizzaFactory::instance().typeForName(canonical);
    if (!resolved.has_value())
        throw ParseError("Parser: unknown pizza type '" + token.lexeme + "'" +
            positionSuffix(token));
    return *resolved;
}

pizza::PizzaSize Parser::parseSize()
{
    const Token token = _lexer.next();
    if (token.kind != TokenKind::Ident)
        throw ParseError("Parser: expected SIZE, got " +
            std::string(tokenKindName(token.kind)) + " '" + token.lexeme + "'" +
            positionSuffix(token));

    const std::string canonical = toLower(token.lexeme);
    if (canonical == "s")   return pizza::PizzaSize::S;
    if (canonical == "m")   return pizza::PizzaSize::M;
    if (canonical == "l")   return pizza::PizzaSize::L;
    if (canonical == "xl")  return pizza::PizzaSize::XL;
    if (canonical == "xxl") return pizza::PizzaSize::XXL;
    throw ParseError("Parser: unknown SIZE '" + token.lexeme + "'" + positionSuffix(token));
}

int Parser::parseNumber()
{
    const Token token = _lexer.next();
    if (token.kind != TokenKind::Number)
        throw ParseError("Parser: expected NUMBER, got " +
            std::string(tokenKindName(token.kind)) + " '" + token.lexeme + "'" +
            positionSuffix(token));
    return token.value;
}

}
