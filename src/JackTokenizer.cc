// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "JackTokenizer.h"
#include <iostream>
#include <stdexcept>
#include <cctype>


namespace hcc {
namespace jack {


namespace {
const std::map<std::string, TokenType> keywordMap = {
    {"class",       TokenType::CLASS},
    {"method",      TokenType::METHOD},
    {"function",    TokenType::FUNCTION},
    {"constructor", TokenType::CONSTRUCTOR},
    {"int",         TokenType::INT},
    {"boolean",     TokenType::BOOLEAN},
    {"char",        TokenType::CHAR},
    {"void",        TokenType::VOID},
    {"var",         TokenType::VAR},
    {"static",      TokenType::STATIC},
    {"field",       TokenType::FIELD},
    {"let",         TokenType::LET},
    {"do",          TokenType::DO},
    {"if",          TokenType::IF},
    {"else",        TokenType::ELSE},
    {"while",       TokenType::WHILE},
    {"return",      TokenType::RETURN},
    {"true",        TokenType::TRUE},
    {"false",       TokenType::FALSE},
    {"null",        TokenType::NULL_},
    {"this",        TokenType::THIS}
};
const std::map<char, TokenType> punctuationMap = {
    {'&',   TokenType::AMPERSAND},
    {'(',   TokenType::PARENTHESIS_LEFT},
    {')',   TokenType::PARENTHESIS_RIGHT},
    {'*',   TokenType::ASTERISK},
    {'+',   TokenType::PLUS_SIGN},
    {',',   TokenType::COMMA},
    {'-',   TokenType::MINUS_SIGN},
    {'.',   TokenType::DOT},
    // slash is intentionally left out
    {';',   TokenType::SEMICOLON},
    {'<',   TokenType::LESS_THAN_SIGN},
    {'=',   TokenType::EQUALS_SIGN},
    {'>',   TokenType::GREATER_THAN_SIGN},
    {'[',   TokenType::BRACKET_LEFT},
    {']',   TokenType::BRACKET_RIGHT},
    {'{',   TokenType::BRACE_LEFT},
    {'|',   TokenType::VERTICAL_BAR},
    {'}',   TokenType::BRACE_RIGHT},
    {'~',   TokenType::TILDE}
};
const std::map<TokenType, std::string> tokenTypeToString = {
    // keyword
    {TokenType::CLASS,              "keyword 'class'"},
    {TokenType::METHOD,             "keyword 'method'"},
    {TokenType::FUNCTION,           "keyword 'function'"},
    {TokenType::CONSTRUCTOR,        "keyword 'constructor'"},
    {TokenType::INT,                "keyword 'int'"},
    {TokenType::BOOLEAN,            "keyword 'boolean'"},
    {TokenType::CHAR,               "keyword 'char'"},
    {TokenType::VOID,               "keyword 'void'"},
    {TokenType::VAR,                "keyword 'var'"},
    {TokenType::STATIC,             "keyword 'static'"},
    {TokenType::FIELD,              "keyword 'field'"},
    {TokenType::LET,                "keyword 'let'"},
    {TokenType::DO,                 "keyword 'do'"},
    {TokenType::IF,                 "keyword 'if'"},
    {TokenType::ELSE,               "keyword 'else'"},
    {TokenType::WHILE,              "keyword 'while'"},
    {TokenType::RETURN,             "keyword 'return'"},
    {TokenType::TRUE,               "keyword 'true'"},
    {TokenType::FALSE,              "keyword 'false'"},
    {TokenType::NULL_,              "keyword 'null'"},
    {TokenType::THIS,               "keyword 'this'"},
    // punctuation
    {TokenType::AMPERSAND,          "'&'"},
    {TokenType::PARENTHESIS_LEFT,   "'('"},
    {TokenType::PARENTHESIS_RIGHT,  "')'"},
    {TokenType::ASTERISK,           "'*'"},
    {TokenType::PLUS_SIGN,          "'+'"},
    {TokenType::COMMA,              "','"},
    {TokenType::MINUS_SIGN,         "'-'"},
    {TokenType::DOT,                "'.'"},
    {TokenType::SLASH,              "'/'"},
    {TokenType::SEMICOLON,          "';'"},
    {TokenType::LESS_THAN_SIGN,     "'<'"},
    {TokenType::EQUALS_SIGN,        "'='"},
    {TokenType::GREATER_THAN_SIGN,  "'>'"},
    {TokenType::BRACKET_LEFT,       "'['"},
    {TokenType::BRACKET_RIGHT,      "']'"},
    {TokenType::BRACE_LEFT,         "'{'"},
    {TokenType::VERTICAL_BAR,       "'|'"},
    {TokenType::BRACE_RIGHT,        "'}'"},
    {TokenType::TILDE,              "'~'"},
    // rest
    {TokenType::IDENTIFIER,         "identifier"},
    {TokenType::INT_CONST,          "integer constant"},
    {TokenType::STRING_CONST,       "string constant"},
    {TokenType::EOF_,               "end of file"},
    {TokenType::STRAY_CHARACTER,    "stray character"}
};
} // end anonymous namespace



std::ostream& operator<<(std::ostream& os, const TokenType& tt)
{
    auto string = tokenTypeToString.find(tt);
    if (string != tokenTypeToString.end()) {
        os << string->second;
        return os;
    }
    throw std::runtime_error("Assertion failed");
}


Tokenizer::Tokenizer(std::basic_istream<char>& is)
    : current(is)
    , line(1)
    , column(0)
{}


TokenType Tokenizer::_advance()
{
    enum class State {
        START,
        SLASH,
        COMMENT_ONELINE,
        COMMENT_MULTILINE,
        COMMENT_MULTILINE_END,
        STRING,
        NUMBER,
        IDENTIFIER
    };
    State state = State::START;

    string = "";

    while (true) {
        char c;
        if (previousChar) {
            c = *previousChar;
            previousChar.reset();
        } else if (current != last) {
            c = *current++;
        } else {
            return TokenType::EOF_;
        }

        if (c == '\n') {
            column = 0;
            ++line;
        } else {
            ++column;
        }

        switch (state) {
        case State::START:
            if (c == '/') {
                state = State::SLASH;
            } else if (c == '"') {
                state = State::STRING;
            } else if (isdigit(c)) {
                intConstant = c - '0';
                state = State::NUMBER;
            } else if (isalpha(c) || c == '_') {
                string += c;
                state = State::IDENTIFIER;
            } else if (isspace(c)) {
                // do nothing
            } else {
                auto it = punctuationMap.find(c);
                return (it != punctuationMap.end()) ? it->second : TokenType::STRAY_CHARACTER;
            }
            break;
        case State::SLASH:
            if (c == '/') {
                state = State::COMMENT_ONELINE;
            } else if (c == '*') {
                state = State::COMMENT_MULTILINE;
            } else {
                previousChar = c;

                return TokenType::SLASH;
            }
            break;
        case State::COMMENT_ONELINE:
            state = (c == '\n') ? State::START : State::COMMENT_ONELINE;
            break;
        case State::COMMENT_MULTILINE:
            state = (c == '*') ? State::COMMENT_MULTILINE_END : State::COMMENT_MULTILINE;
            break;
        case State::COMMENT_MULTILINE_END:
            state = (c == '/') ? State::START : State::COMMENT_MULTILINE;
            break;
        case State::STRING:
            if (c == '"') {
                return TokenType::STRING_CONST;
            } else {
                string += c;
            }
            break;
        case State::NUMBER:
            if (isdigit(c)) {
                intConstant = 10*intConstant + (c - '0');
            } else {
                previousChar = c;
                return TokenType::INT_CONST;
            }
            break;
        case State::IDENTIFIER:
            if (isalnum(c) || c == '_') {
                string += c;
            } else {
                previousChar = c;
                auto kw = keywordMap.find(string);
                return (kw != keywordMap.end()) ? kw->second : TokenType::IDENTIFIER;
            }
            break;
        }
    }
}


} // end namespace jack
} // end namespace hcc
