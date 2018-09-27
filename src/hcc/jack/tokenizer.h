// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#pragma once

#include <cassert>
#include <iterator>
#include <string>

namespace hcc {
namespace jack {

enum class token_type {
    // keyword
    BOOLEAN,
    CHAR,
    CLASS,
    CONSTRUCTOR,
    DO,
    ELSE,
    FALSE,
    FIELD,
    FUNCTION,
    IF,
    INT,
    LET,
    METHOD,
    NULL_,
    RETURN,
    STATIC,
    THIS,
    TRUE,
    VAR,
    VOID,
    WHILE,
    // punctuation
    AMPERSAND,
    PARENTHESIS_LEFT,
    PARENTHESIS_RIGHT,
    ASTERISK,
    PLUS_SIGN,
    COMMA,
    MINUS_SIGN,
    DOT,
    SLASH,
    SEMICOLON,
    LESS_THAN_SIGN,
    EQUALS_SIGN,
    GREATER_THAN_SIGN,
    BRACKET_LEFT,
    BRACKET_RIGHT,
    BRACE_LEFT,
    VERTICAL_BAR,
    BRACE_RIGHT,
    TILDE,
    // rest
    IDENTIFIER,
    INT_CONST,
    STRING_CONST,
    EOF_,
    STRAY_CHARACTER,
    UNTERMINATED_COMMENT,
    UNTERMINATED_STRING_CONST,
};

std::string to_string(const token_type& tt);

struct token {
    token_type type{token_type::EOF_};
    std::string string_value;
    int int_value{0};
};

struct position {
    unsigned line{1};
    unsigned column{0};

    void update(char c)
    {
        if (c == '\n') {
            column = 0;
            ++line;
        } else {
            ++column;
        }
    }
};

struct tokenizer {
    position pos;

    explicit tokenizer(std::istreambuf_iterator<char> current, std::istreambuf_iterator<char> last)
        : current(current)
        , last(last)
    {
    }

    token advance();

    void put_char(char c)
    {
        assert(!has_previous_char);
        previous_char = c;
        has_previous_char = true;
    }

    bool get_char(char& c)
    {
        auto ret = get_char_(c);
        if (ret) {
            pos.update(c);
        }
        return ret;
    }
    bool get_char_(char& c)
    {
        if (has_previous_char) {
            c = previous_char;
            has_previous_char = false;
            return true;
        }

        if (current != last) {
            c = *current++;
            return true;
        }

        return false;
    }

    char previous_char;
    bool has_previous_char{false};
    std::istreambuf_iterator<char> current;
    std::istreambuf_iterator<char> last;
};

} // namespace jack {
} // namespace hcc {
