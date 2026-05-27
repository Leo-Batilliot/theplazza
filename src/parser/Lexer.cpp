#include "parser/Lexer.hpp"

#include "common/Errors.hpp"

#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>

namespace plazza::parser {

namespace {

bool isAlpha(char c) noexcept
{
    return std::isalpha(static_cast<unsigned char>(c)) != 0;
}

bool isDigit(char c) noexcept
{
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool isHorizontalSpace(char c) noexcept
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

}

Lexer::Lexer(std::string source) : _source(std::move(source)), _position(0) {}

char Lexer::lookAhead(std::size_t offset) const noexcept
{
    const std::size_t target = _position + offset;
    return target < _source.size() ? _source[target] : '\0';
}

void Lexer::skipWhitespace()
{
    while (!atEnd() && isHorizontalSpace(current()))
        ++_position;
}

Token Lexer::scanOne()
{
    skipWhitespace();
    if (atEnd())
        return Token{TokenKind::End, "", _position, 0};

    const std::size_t start = _position;
    const char c = current();

    if (c == ';') {
        ++_position;
        return Token{TokenKind::Semicolon, ";", start, 0};
    }

    if ((c == 'x' || c == 'X') && isDigit(lookAhead(1))) {
        ++_position;
        if (current() == '0')
            throw ParseError("Lexer: NUMBER cannot start with 0 at position " +
                std::to_string(start));
        const std::size_t digitsStart = _position;
        while (!atEnd() && isDigit(current()))
            ++_position;
        const std::string digits = _source.substr(digitsStart, _position - digitsStart);
        int value = 0;
        try {
            value = std::stoi(digits);
        } catch (const std::out_of_range &) {
            throw ParseError("Lexer: NUMBER overflow at position " + std::to_string(start));
        }
        return Token{TokenKind::Number, _source.substr(start, _position - start), start, value};
    }

    if (isAlpha(c)) {
        while (!atEnd() && isAlpha(current()))
            ++_position;
        return Token{TokenKind::Ident, _source.substr(start, _position - start), start, 0};
    }

    throw ParseError("Lexer: unexpected character '" + std::string(1, c) +
        "' at position " + std::to_string(start));
}

Token Lexer::next()
{
    if (_lookahead.has_value()) {
        Token token = std::move(*_lookahead);
        _lookahead.reset();
        return token;
    }
    return scanOne();
}

const Token &Lexer::peek()
{
    if (!_lookahead.has_value())
        _lookahead = scanOne();
    return *_lookahead;
}

bool Lexer::eof()
{
    return peek().kind == TokenKind::End;
}

}
