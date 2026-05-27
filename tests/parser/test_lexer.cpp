#include <criterion/criterion.h>

#include "common/Errors.hpp"
#include "parser/Lexer.hpp"
#include "parser/Token.hpp"

#include <string>
#include <vector>

using plazza::parser::Lexer;
using plazza::parser::Token;
using plazza::parser::TokenKind;

static std::vector<Token> tokenize(const std::string &source)
{
    Lexer lexer(source);
    std::vector<Token> tokens;
    while (true) {
        Token t = lexer.next();
        const bool end = t.kind == TokenKind::End;
        tokens.push_back(std::move(t));
        if (end)
            break;
    }
    return tokens;
}

Test(lexer_basics, empty_input_yields_end_only)
{
    auto tokens = tokenize("");

    cr_assert_eq(tokens.size(), 1u);
    cr_assert_eq(tokens[0].kind, TokenKind::End);
}

Test(lexer_basics, whitespace_only_yields_end_only)
{
    auto tokens = tokenize("   \t  \t");

    cr_assert_eq(tokens.size(), 1u);
    cr_assert_eq(tokens[0].kind, TokenKind::End);
}

Test(lexer_ident, single_ident_is_recognized)
{
    auto tokens = tokenize("regina");

    cr_assert_eq(tokens.size(), 2u);
    cr_assert_eq(tokens[0].kind, TokenKind::Ident);
    cr_assert_str_eq(tokens[0].lexeme.c_str(), "regina");
    cr_assert_eq(tokens[1].kind, TokenKind::End);
}

Test(lexer_ident, ident_preserves_case)
{
    auto tokens = tokenize("XXL");

    cr_assert_eq(tokens[0].kind, TokenKind::Ident);
    cr_assert_str_eq(tokens[0].lexeme.c_str(), "XXL");
}

Test(lexer_ident, ident_starting_with_x_then_letter_is_ident)
{
    auto tokens = tokenize("xyzpizza");

    cr_assert_eq(tokens[0].kind, TokenKind::Ident);
    cr_assert_str_eq(tokens[0].lexeme.c_str(), "xyzpizza");
}

Test(lexer_number, simple_number_has_value)
{
    auto tokens = tokenize("x1");

    cr_assert_eq(tokens[0].kind, TokenKind::Number);
    cr_assert_str_eq(tokens[0].lexeme.c_str(), "x1");
    cr_assert_eq(tokens[0].value, 1);
}

Test(lexer_number, multi_digit_number)
{
    auto tokens = tokenize("x42");

    cr_assert_eq(tokens[0].kind, TokenKind::Number);
    cr_assert_eq(tokens[0].value, 42);
}

Test(lexer_number, uppercase_X_prefix_also_accepted)
{
    auto tokens = tokenize("X7");

    cr_assert_eq(tokens[0].kind, TokenKind::Number);
    cr_assert_eq(tokens[0].value, 7);
}

Test(lexer_number, leading_zero_throws_parse_error)
{
    cr_assert_throw(tokenize("x0"), plazza::ParseError);
}

Test(lexer_number, leading_zero_in_multidigit_throws)
{
    cr_assert_throw(tokenize("x01"), plazza::ParseError);
}

Test(lexer_number, overflow_throws_parse_error)
{
    cr_assert_throw(tokenize("x99999999999999999999"), plazza::ParseError);
}

Test(lexer_semicolon, semicolon_is_its_own_token)
{
    auto tokens = tokenize(";");

    cr_assert_eq(tokens[0].kind, TokenKind::Semicolon);
    cr_assert_str_eq(tokens[0].lexeme.c_str(), ";");
    cr_assert_eq(tokens[1].kind, TokenKind::End);
}

Test(lexer_invalid, unexpected_character_throws)
{
    cr_assert_throw(tokenize("@"), plazza::ParseError);
    cr_assert_throw(tokenize("regina M ?"), plazza::ParseError);
    cr_assert_throw(tokenize("5"), plazza::ParseError);
}

Test(lexer_full_order, valid_single_order_tokenizes)
{
    auto tokens = tokenize("regina XXL x2");

    cr_assert_eq(tokens.size(), 4u);
    cr_assert_eq(tokens[0].kind, TokenKind::Ident);
    cr_assert_str_eq(tokens[0].lexeme.c_str(), "regina");
    cr_assert_eq(tokens[1].kind, TokenKind::Ident);
    cr_assert_str_eq(tokens[1].lexeme.c_str(), "XXL");
    cr_assert_eq(tokens[2].kind, TokenKind::Number);
    cr_assert_eq(tokens[2].value, 2);
    cr_assert_eq(tokens[3].kind, TokenKind::End);
}

Test(lexer_full_order, multi_order_with_semicolons)
{
    auto tokens = tokenize("regina XXL x2; fantasia M x3; margarita S x1");

    cr_assert_eq(tokens.size(), 12u);
    cr_assert_eq(tokens[0].kind, TokenKind::Ident);
    cr_assert_eq(tokens[3].kind, TokenKind::Semicolon);
    cr_assert_eq(tokens[7].kind, TokenKind::Semicolon);
    cr_assert_eq(tokens[11].kind, TokenKind::End);
    cr_assert_str_eq(tokens[4].lexeme.c_str(), "fantasia");
    cr_assert_eq(tokens[6].value, 3);
    cr_assert_eq(tokens[10].value, 1);
}

Test(lexer_full_order, no_whitespace_between_tokens_still_lex)
{
    auto tokens = tokenize("regina;fantasia");

    cr_assert_eq(tokens.size(), 4u);
    cr_assert_eq(tokens[0].kind, TokenKind::Ident);
    cr_assert_eq(tokens[1].kind, TokenKind::Semicolon);
    cr_assert_eq(tokens[2].kind, TokenKind::Ident);
    cr_assert_eq(tokens[3].kind, TokenKind::End);
}

Test(lexer_positions, position_tracks_token_start)
{
    Lexer lexer("regina  M x2");

    Token t1 = lexer.next();
    Token t2 = lexer.next();
    Token t3 = lexer.next();

    cr_assert_eq(t1.position, 0u);
    cr_assert_eq(t2.position, 8u);
    cr_assert_eq(t3.position, 10u);
}

Test(lexer_peek, peek_does_not_consume)
{
    Lexer lexer("regina");

    const Token &p1 = lexer.peek();
    cr_assert_eq(p1.kind, TokenKind::Ident);
    cr_assert_str_eq(p1.lexeme.c_str(), "regina");

    const Token &p2 = lexer.peek();
    cr_assert_eq(p2.kind, TokenKind::Ident);
    cr_assert_str_eq(p2.lexeme.c_str(), "regina");

    Token n = lexer.next();
    cr_assert_eq(n.kind, TokenKind::Ident);
    cr_assert_str_eq(n.lexeme.c_str(), "regina");

    cr_assert(lexer.eof());
}

Test(lexer_peek, eof_true_when_only_end_remains)
{
    Lexer lexer("");

    cr_assert(lexer.eof());
    Token t = lexer.next();
    cr_assert_eq(t.kind, TokenKind::End);
    cr_assert(lexer.eof());
}

Test(lexer_double_semicolon, empty_segment_produces_back_to_back_semicolons)
{
    auto tokens = tokenize("regina M x1;;");

    cr_assert_eq(tokens.size(), 6u);
    cr_assert_eq(tokens[3].kind, TokenKind::Semicolon);
    cr_assert_eq(tokens[4].kind, TokenKind::Semicolon);
    cr_assert_eq(tokens[5].kind, TokenKind::End);
}
