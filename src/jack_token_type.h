// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#pragma once
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

} // namespace jack {
} // namespace hcc {
