// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "jack_tokenizer.h"
#include <cassert>
#include <cctype>
#include <map>
#include <string>

namespace {

using hcc::jack::token;
using hcc::jack::token_type;
using hcc::jack::tokenizer;

const std::map<std::string, token_type> keyword_map = {
    {"boolean", token_type::BOOLEAN},
    {"char", token_type::CHAR},
    {"class", token_type::CLASS},
    {"constructor", token_type::CONSTRUCTOR},
    {"do", token_type::DO},
    {"else", token_type::ELSE},
    {"false", token_type::FALSE},
    {"field", token_type::FIELD},
    {"function", token_type::FUNCTION},
    {"if", token_type::IF},
    {"int", token_type::INT},
    {"let", token_type::LET},
    {"method", token_type::METHOD},
    {"null", token_type::NULL_},
    {"return", token_type::RETURN},
    {"static", token_type::STATIC},
    {"this", token_type::THIS},
    {"true", token_type::TRUE},
    {"var", token_type::VAR},
    {"void", token_type::VOID},
    {"while", token_type::WHILE},
};

const std::map<char, token_type> punctuation_map = {
    {'&', token_type::AMPERSAND},
    {'(', token_type::PARENTHESIS_LEFT},
    {')', token_type::PARENTHESIS_RIGHT},
    {'*', token_type::ASTERISK},
    {'+', token_type::PLUS_SIGN},
    {',', token_type::COMMA},
    {'-', token_type::MINUS_SIGN},
    {'.', token_type::DOT},
    // slash is intentionally left out
    {';', token_type::SEMICOLON},
    {'<', token_type::LESS_THAN_SIGN},
    {'=', token_type::EQUALS_SIGN},
    {'>', token_type::GREATER_THAN_SIGN},
    {'[', token_type::BRACKET_LEFT},
    {']', token_type::BRACKET_RIGHT},
    {'{', token_type::BRACE_LEFT},
    {'|', token_type::VERTICAL_BAR},
    {'}', token_type::BRACE_RIGHT},
    {'~', token_type::TILDE},
};

// Idiomatic type for function returning pointer to itself
struct transition_;
using transition = transition_ (*)(tokenizer&, token&);
struct transition_ {
    transition_(transition pp)
        : p(pp)
    {
    }
    operator transition() { return p; }

private:
    transition p;
};

// Tokenizer is implemented as a finite state machine
transition_ start(tokenizer&, token&);
transition_ slash(tokenizer&, token&);
transition_ comment_oneline(tokenizer&, token&);
transition_ comment_multiline(tokenizer&, token&);
transition_ comment_multiline_end(tokenizer&, token&);
transition_ string(tokenizer&, token&);
transition_ number(tokenizer&, token&);
transition_ identifier(tokenizer&, token&);
transition_ finish(tokenizer&, token&) { assert(false); }

transition_ start(tokenizer& tokenizer_, token& token_)
{
    char c;
    if (!tokenizer_.get_char(c)) {
        token_.type = token_type::EOF_;
        return finish;
    }

    if (c == '/') {
        return slash;
    }

    if (c == '"') {
        return string;
    }

    if (isdigit(c)) {
        token_.int_value = c - '0';
        return number;
    }

    if (isalpha(c) || c == '_') {
        token_.string_value += c;
        return identifier;
    }

    if (isspace(c)) {
        return start;
    }

    auto it = punctuation_map.find(c);
    if (it != punctuation_map.end()) {
        token_.type = it->second;
        return finish;
    }

    token_.type = token_type::STRAY_CHARACTER;
    return finish;
}

transition_ slash(tokenizer& tokenizer_, token& token_)
{
    char c;
    if (!tokenizer_.get_char(c)) {
        token_.type = token_type::SLASH;
        return finish;
    }

    if (c == '/') {
        return comment_oneline;
    }

    if (c == '*') {
        return comment_multiline;
    }

    tokenizer_.put_char(c);
    token_.type = token_type::SLASH;
    return finish;
}

transition_ comment_oneline(tokenizer& tokenizer_, token& token_)
{
    char c;
    if (!tokenizer_.get_char(c)) {
        return start;
    }

    if (c == '\n') {
        return start;
    }

    return comment_oneline;
}

transition_ comment_multiline(tokenizer& tokenizer_, token& token_)
{
    char c;
    if (!tokenizer_.get_char(c)) {
        token_.type = token_type::UNTERMINATED_COMMENT;
        return finish;
    }

    if (c == '*') {
        return comment_multiline_end;
    }

    return comment_multiline;
}

transition_ comment_multiline_end(tokenizer& tokenizer_, token& token_)
{
    char c;
    if (!tokenizer_.get_char(c)) {
        token_.type = token_type::UNTERMINATED_COMMENT;
        return finish;
    }

    if (c == '/') {
        return start;
    }

    return comment_multiline;
}

transition_ string(tokenizer& tokenizer_, token& token_)
{
    char c;
    if (!tokenizer_.get_char(c)) {
        token_.type = token_type::UNTERMINATED_STRING_CONST;
        return finish;
    }

    if (c == '"') {
        token_.type = token_type::STRING_CONST;
        return finish;
    }

    token_.string_value += c;
    return string;
}

transition_ number(tokenizer& tokenizer_, token& token_)
{
    char c;
    if (!tokenizer_.get_char(c)) {
        token_.type = token_type::INT_CONST;
        return finish;
    }

    if (isdigit(c)) {
        token_.int_value = 10 * token_.int_value + (c - '0');
        return number;
    } else {
        tokenizer_.put_char(c);
        token_.type = token_type::INT_CONST;
        return finish;
    }
}

transition_ identifier(tokenizer& tokenizer_, token& token_)
{
    char c;
    if (!tokenizer_.get_char(c)) {
        auto kw = keyword_map.find(token_.string_value);
        token_.type = (kw != keyword_map.end()) ? kw->second : token_type::IDENTIFIER;
        return finish;
    }

    if (isalnum(c) || c == '_') {
        token_.string_value += c;
        return identifier;
    } else {
        tokenizer_.put_char(c);
        auto kw = keyword_map.find(token_.string_value);
        token_.type = (kw != keyword_map.end()) ? kw->second : token_type::IDENTIFIER;
        return finish;
    }
}

} // anonymous namespace

namespace hcc {
namespace jack {

token tokenizer::advance()
{
    token t;
    transition f = start;
    while (f != finish) {
        f = f(*this, t);
    }
    return t;
}

} // end namespace jack
} // end namespace hcc
