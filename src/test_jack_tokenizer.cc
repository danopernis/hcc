// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "jack_tokenizer.h"
#include <cassert>
#include <sstream>
#include <vector>


using hcc::jack::token;
using hcc::jack::token_type;
using hcc::jack::tokenizer;


std::vector<token> driver(const std::string& s)
{
    std::vector<token> result;

    std::stringbuf input {s};
    tokenizer t {
        std::istreambuf_iterator<char>(&input),
        std::istreambuf_iterator<char>()};

    while (true) {
        result.push_back(t.advance());
        if (result.back().type == token_type::EOF_) {
            return result;
        }
    }
}

void test_empty(const std::string& s)
{
    const auto tokens = driver(s);
    assert (tokens.size() == 1);
    assert (tokens[0].type == token_type::EOF_);
}

void test_just_quote()
{
    const auto tokens = driver("\"");
    assert (tokens.size() == 2);
    assert (tokens[0].type == token_type::UNTERMINATED_STRING_CONST);
    assert (tokens[1].type == token_type::EOF_);
}

void test_just_digit(char c)
{
    std::string s(1, c);
    const auto tokens = driver(s);
    assert (tokens.size() == 2);
    assert (tokens[0].type == token_type::INT_CONST);
    assert (tokens[0].int_value == std::stoi(s));
    assert (tokens[1].type == token_type::EOF_);
}

void test_just_identifier(char c)
{
    std::string s(1, c);
    const auto tokens = driver(s);
    assert (tokens.size() == 2);
    assert (tokens[0].type == token_type::IDENTIFIER);
    assert (tokens[0].string_value == s);
    assert (tokens[1].type == token_type::EOF_);
}

void test_just_space(char c)
{
    std::string s(1, c);
    const auto tokens = driver(s);
    assert (tokens.size() == 1);
    assert (tokens[0].type == token_type::EOF_);
}

void test_just_punctuation(char c, const token_type& tt)
{
    std::string s(1, c);
    const auto tokens = driver(s);
    assert (tokens.size() == 2);
    assert (tokens[0].type == tt);
    assert (tokens[1].type == token_type::EOF_);
}

auto hello_program = R"(
// Hello world program
class Main {
    function void main() {
        do Output.printString("Hello world!");
        return;
    }
}
)";
void test_hello()
{
    const auto tokens = driver(hello_program);
    assert (tokens.size() == 22);
    assert (tokens[ 0].type == token_type::CLASS);
    assert (tokens[ 1].type == token_type::IDENTIFIER);
    assert (tokens[ 1].string_value == "Main");
    assert (tokens[ 2].type == token_type::BRACE_LEFT);
    assert (tokens[ 3].type == token_type::FUNCTION);
    assert (tokens[ 4].type == token_type::VOID);
    assert (tokens[ 5].type == token_type::IDENTIFIER);
    assert (tokens[ 5].string_value == "main");
    assert (tokens[ 6].type == token_type::PARENTHESIS_LEFT);
    assert (tokens[ 7].type == token_type::PARENTHESIS_RIGHT);
    assert (tokens[ 8].type == token_type::BRACE_LEFT);
    assert (tokens[ 9].type == token_type::DO);
    assert (tokens[10].type == token_type::IDENTIFIER);
    assert (tokens[10].string_value == "Output");
    assert (tokens[11].type == token_type::DOT);
    assert (tokens[12].type == token_type::IDENTIFIER);
    assert (tokens[12].string_value == "printString");
    assert (tokens[13].type == token_type::PARENTHESIS_LEFT);
    assert (tokens[14].type == token_type::STRING_CONST);
    assert (tokens[14].string_value == "Hello world!");
    assert (tokens[15].type == token_type::PARENTHESIS_RIGHT);
    assert (tokens[16].type == token_type::SEMICOLON);
    assert (tokens[17].type == token_type::RETURN);
    assert (tokens[18].type == token_type::SEMICOLON);
    assert (tokens[19].type == token_type::BRACE_RIGHT);
    assert (tokens[20].type == token_type::BRACE_RIGHT);
    assert (tokens[21].type == token_type::EOF_);
}

int main()
{
    const auto digits = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    const auto alpha_underscore = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '_'};
    const auto space = {
        ' ', '\t', '\n'};

    test_empty("");
    test_just_quote();
    for (const char c : digits) {
        test_just_digit(c);
    }
    for (const char c : alpha_underscore) {
        test_just_identifier(c);
    }
    for (const char c : space) {
        test_just_space(c);
    }
    test_just_punctuation('&', token_type::AMPERSAND);
    test_just_punctuation('(', token_type::PARENTHESIS_LEFT);
    test_just_punctuation(')', token_type::PARENTHESIS_RIGHT);
    test_just_punctuation('*', token_type::ASTERISK);
    test_just_punctuation('+', token_type::PLUS_SIGN);
    test_just_punctuation(',', token_type::COMMA);
    test_just_punctuation('-', token_type::MINUS_SIGN);
    test_just_punctuation('.', token_type::DOT);
    test_just_punctuation('/', token_type::SLASH);
    test_just_punctuation(';', token_type::SEMICOLON);
    test_just_punctuation('<', token_type::LESS_THAN_SIGN);
    test_just_punctuation('=', token_type::EQUALS_SIGN);
    test_just_punctuation('>', token_type::GREATER_THAN_SIGN);
    test_just_punctuation('[', token_type::BRACKET_LEFT);
    test_just_punctuation(']', token_type::BRACKET_RIGHT);
    test_just_punctuation('{', token_type::BRACE_LEFT);
    test_just_punctuation('|', token_type::VERTICAL_BAR);
    test_just_punctuation('}', token_type::BRACE_RIGHT);
    test_just_punctuation('~', token_type::TILDE);
    test_just_punctuation('@', token_type::STRAY_CHARACTER);

    test_empty("//");
    test_empty("//*");
    test_empty("//1");
    test_empty("//\n");
    test_empty("/**/");
    test_empty("/*/*/");
    test_empty("/*1*/");
    test_empty("/*\n*/");
    test_empty("/**/\n");

    test_hello();

    return 0;
}
