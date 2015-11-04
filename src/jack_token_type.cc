// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "jack_token_type.h"
#include <cassert>
#include <map>
#include <string>

namespace {

using hcc::jack::token_type;

const std::map<token_type, std::string> lookup = {
    // keyword
    {token_type::BOOLEAN, "keyword 'boolean'"},
    {token_type::CHAR, "keyword 'char'"},
    {token_type::CLASS, "keyword 'class'"},
    {token_type::CONSTRUCTOR, "keyword 'constructor'"},
    {token_type::DO, "keyword 'do'"},
    {token_type::ELSE, "keyword 'else'"},
    {token_type::FALSE, "keyword 'false'"},
    {token_type::FIELD, "keyword 'field'"},
    {token_type::FUNCTION, "keyword 'function'"},
    {token_type::IF, "keyword 'if'"},
    {token_type::INT, "keyword 'int'"},
    {token_type::LET, "keyword 'let'"},
    {token_type::METHOD, "keyword 'method'"},
    {token_type::NULL_, "keyword 'null'"},
    {token_type::RETURN, "keyword 'return'"},
    {token_type::STATIC, "keyword 'static'"},
    {token_type::THIS, "keyword 'this'"},
    {token_type::TRUE, "keyword 'true'"},
    {token_type::VAR, "keyword 'var'"},
    {token_type::VOID, "keyword 'void'"},
    {token_type::WHILE, "keyword 'while'"},
    // punctuation
    {token_type::AMPERSAND, "'&'"},
    {token_type::PARENTHESIS_LEFT, "'('"},
    {token_type::PARENTHESIS_RIGHT, "')'"},
    {token_type::ASTERISK, "'*'"},
    {token_type::PLUS_SIGN, "'+'"},
    {token_type::COMMA, "','"},
    {token_type::MINUS_SIGN, "'-'"},
    {token_type::DOT, "'.'"},
    {token_type::SLASH, "'/'"},
    {token_type::SEMICOLON, "';'"},
    {token_type::LESS_THAN_SIGN, "'<'"},
    {token_type::EQUALS_SIGN, "'='"},
    {token_type::GREATER_THAN_SIGN, "'>'"},
    {token_type::BRACKET_LEFT, "'['"},
    {token_type::BRACKET_RIGHT, "']'"},
    {token_type::BRACE_LEFT, "'{'"},
    {token_type::VERTICAL_BAR, "'|'"},
    {token_type::BRACE_RIGHT, "'}'"},
    {token_type::TILDE, "'~'"},
    // rest
    {token_type::IDENTIFIER, "identifier"},
    {token_type::INT_CONST, "integer constant"},
    {token_type::STRING_CONST, "string constant"},
    {token_type::EOF_, "end of file"},
    {token_type::STRAY_CHARACTER, "stray character"},
    {token_type::UNTERMINATED_COMMENT, "unterminated comment"},
    {token_type::UNTERMINATED_STRING_CONST, "unterminated string constant"},
};

} // namespace {

namespace hcc {
namespace jack {

std::string to_string(const token_type& tt)
{
    const auto it = lookup.find(tt);
    assert(it != lookup.end());
    return it->second;
}

} // namespace jack {
} // namespace hcc {
