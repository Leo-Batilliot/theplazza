#pragma once

#include "parser/Token.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace plazza::parser {

class Lexer {
public:
    explicit Lexer(std::string source);

    Lexer(const Lexer &) = delete;
    Lexer &operator=(const Lexer &) = delete;
    Lexer(Lexer &&) noexcept = default;
    Lexer &operator=(Lexer &&) noexcept = default;
    ~Lexer() = default;

    Token next();
    const Token &peek();
    [[nodiscard]] bool eof();

private:
    Token scanOne();
    void skipWhitespace();
    [[nodiscard]] bool atEnd() const noexcept { return _position >= _source.size(); }
    [[nodiscard]] char current() const noexcept { return _source[_position]; }
    [[nodiscard]] char lookAhead(std::size_t offset) const noexcept;

    std::string _source;
    std::size_t _position;
    std::optional<Token> _lookahead;
};

}
