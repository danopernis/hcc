// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/ssa/tokenizer.h"

#include <cassert>
#include <map>
#include <sstream>
#include <stdexcept>

namespace hcc {
namespace ssa {
namespace {

const std::map<std::string, token_type> string_to_token = {
    {"global", token_type::GLOBAL},
    {"define", token_type::DEFINE},
    {"block", token_type::BLOCK},
    {"return", token_type::RETURN},
    {"jump", token_type::JUMP},
    {"jlt", token_type::JLT},
    {"jeq", token_type::JEQ},
    {"call", token_type::CALL},
    {"argument", token_type::ARGUMENT},
    {"load", token_type::LOAD},
    {"store", token_type::STORE},
    {"mov", token_type::MOV},
    {"add", token_type::ADD},
    {"sub", token_type::SUB},
    {"and", token_type::AND},
    {"or", token_type::OR},
    {"neg", token_type::NEG},
    {"not", token_type::NOT},
    {"phi", token_type::PHI},
};

enum class state {
    START,
    CONSTANT,
    REG,
    GLOBAL,
    LOCAL,
    LABEL,
    KEYWORD,
};

token_type keyword_to_token_type(const token& t)
{
    try {
        return string_to_token.at(t.token);
    }
    catch (const std::exception& e) {
        std::stringstream message;
        message << "Unexpected '" << t.token << "' at line " << t.line_number;
        throw std::runtime_error(message.str());
    }
}

token_type get_token_type(const state& current_state, const token& t)
{
    switch (current_state) {
    case state::CONSTANT:
        return token_type::CONSTANT;
    case state::REG:
        return token_type::REG;
    case state::GLOBAL:
        return token_type::GLOBAL;
    case state::LOCAL:
        return token_type::LOCAL;
    case state::LABEL:
        return token_type::LABEL;
    case state::KEYWORD:
        return keyword_to_token_type(t);
    case state::START:
        assert(false);
    }
    assert(false);
}

std::istreambuf_iterator<char> last;

} // namespace {

std::string to_string(const token_type& tt)
{
    switch (tt) {
    case token_type::CONSTANT:
        return "#integer constant";
    case token_type::REG:
        return "%register";
    case token_type::GLOBAL:
        return "@global";
    case token_type::LOCAL:
        return "&local";
    case token_type::LABEL:
        return "$label";
    case token_type::DEFINE:
        return "define";
    case token_type::BLOCK:
        return "block";
    case token_type::RETURN:
        return "return";
    case token_type::JUMP:
        return "jump";
    case token_type::JLT:
        return "jlt";
    case token_type::JEQ:
        return "jeq";
    case token_type::CALL:
        return "call";
    case token_type::ARGUMENT:
        return "argument";
    case token_type::LOAD:
        return "load";
    case token_type::STORE:
        return "store";
    case token_type::MOV:
        return "mov";
    case token_type::ADD:
        return "add";
    case token_type::SUB:
        return "sub";
    case token_type::AND:
        return "and";
    case token_type::OR:
        return "or";
    case token_type::NEG:
        return "neg";
    case token_type::NOT:
        return "not";
    case token_type::PHI:
        return "phi";
    case token_type::SEMICOLON:
        return ";";
    case token_type::END:
        return "end of file";
    }
    assert(false);
}

token_type next_impl(std::istreambuf_iterator<char>& current, unsigned& line_number, token& t)
{
    auto current_state = state::START;

    while (true) {
        if (current == last) {
            return token_type::END;
        }
        const auto c = *current++;
        if (c == '\n') {
            ++line_number;
        }

        switch (current_state) {
        case state::START:
            t.token = "";
            if (c == '#') {
                current_state = state::CONSTANT;
            } else if (c == '%') {
                current_state = state::REG;
            } else if (c == '@') {
                current_state = state::GLOBAL;
            } else if (c == '&') {
                current_state = state::LOCAL;
            } else if (c == '$') {
                current_state = state::LABEL;
            } else if (c == ';') {
                return token_type::SEMICOLON;
            } else if (isspace(c)) {
                // just skip
            } else {
                current_state = state::KEYWORD;
                t.token.push_back(c);
            }
            break;
        case state::CONSTANT:
        case state::REG:
        case state::GLOBAL:
        case state::LOCAL:
        case state::LABEL:
        case state::KEYWORD:
            t.line_number = line_number;
            if (isspace(c)) {
                return get_token_type(current_state, t);
            } else {
                t.token.push_back(c);
            }
            break;
        }
    }
}

token tokenizer::next()
{
    token t;
    t.type = next_impl(current, line_number, t);
    return t;
}

} // namespace ssa {
} // namespace hcc {
