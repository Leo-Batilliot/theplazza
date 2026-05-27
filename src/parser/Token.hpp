#pragma once

#include <cstddef>
#include <string>

namespace plazza::parser {

enum class TokenKind {
    Ident,
    Number,
    Semicolon,
    End,
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    std::size_t position;
    int value;
};

[[nodiscard]] inline const char *tokenKindName(TokenKind kind) noexcept
{
    switch (kind) {
        case TokenKind::Ident:     return "Ident";
        case TokenKind::Number:    return "Number";
        case TokenKind::Semicolon: return "Semicolon";
        case TokenKind::End:       return "End";
    }
    return "Unknown";
}

}
