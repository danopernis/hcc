// Copyright (c) 2013-2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <sstream>

namespace {

using hcc::ssa::basic_block;
using hcc::ssa::instruction_list;
using hcc::ssa::instruction_type;
using hcc::ssa::subroutine;
using hcc::ssa::unit;

enum class ssa_token_type {
    GLOBAL,
    DEFINE,
    BLOCK,
    RETURN,
    BRANCH,
    JUMP,
    CALL,
    ARGUMENT,
    LOAD,
    STORE,
    MOV,
    ADD,
    SUB,
    AND,
    OR,
    LT,
    GT,
    EQ,
    NEG,
    NOT,
    PHI,
    END,
};

const std::map<ssa_token_type, std::string> token_to_string = {
    { ssa_token_type::GLOBAL,   "global" },
    { ssa_token_type::DEFINE,   "define" },
    { ssa_token_type::BLOCK,    "block" },
    { ssa_token_type::RETURN,   "return" },
    { ssa_token_type::BRANCH,   "branch" },
    { ssa_token_type::JUMP,     "jump" },
    { ssa_token_type::CALL,     "call" },
    { ssa_token_type::ARGUMENT, "argument" },
    { ssa_token_type::LOAD,     "load" },
    { ssa_token_type::STORE,    "store" },
    { ssa_token_type::MOV,      "mov" },
    { ssa_token_type::ADD,      "add" },
    { ssa_token_type::SUB,      "sub" },
    { ssa_token_type::AND,      "and" },
    { ssa_token_type::OR,       "or" },
    { ssa_token_type::LT,       "lt" },
    { ssa_token_type::GT,       "gt" },
    { ssa_token_type::EQ,       "eq" },
    { ssa_token_type::NEG,      "neg" },
    { ssa_token_type::NOT,      "not" },
    { ssa_token_type::PHI,      "phi" },
    { ssa_token_type::END,      "end of file" },
};

const std::map<std::string, ssa_token_type> string_to_token = {
    { "global",     ssa_token_type::GLOBAL },
    { "define",     ssa_token_type::DEFINE },
    { "block",      ssa_token_type::BLOCK },
    { "return",     ssa_token_type::RETURN },
    { "branch",     ssa_token_type::BRANCH },
    { "jump",       ssa_token_type::JUMP },
    { "call",       ssa_token_type::CALL },
    { "argument",   ssa_token_type::ARGUMENT },
    { "load",       ssa_token_type::LOAD },
    { "store",      ssa_token_type::STORE },
    { "mov",        ssa_token_type::MOV },
    { "add",        ssa_token_type::ADD },
    { "sub",        ssa_token_type::SUB },
    { "and",        ssa_token_type::AND },
    { "or",         ssa_token_type::OR },
    { "lt",         ssa_token_type::LT },
    { "gt",         ssa_token_type::GT },
    { "eq",         ssa_token_type::EQ },
    { "neg",        ssa_token_type::NEG },
    { "not",        ssa_token_type::NOT },
    { "phi",        ssa_token_type::PHI },
    // END deliberately missing
};

struct ssa_tokenizer {
    ssa_tokenizer(std::istream& input) : input(input) { }

    void advance()
    {
        std::string line;
        if (!std::getline(input, line)) {
            token_type = ssa_token_type::END;
            return;
        }

        ++line_number;
        std::stringstream line_stream(line);

        std::string token;
        line_stream >> token;

        const auto it = string_to_token.find(token);
        if (it == string_to_token.end()) {
            std::stringstream message;
            message
                << "Unexpected '" << token
                << "' at line " << line_number;
            throw std::runtime_error(message.str());
        }
        token_type = it->second;

        arguments.clear();
        while (true) {
            std::string token;
            line_stream >> token;
            if (token.empty()) {
                break;
            }
            arguments.push_back(std::move(token));
        }
    }

    ssa_token_type token_type;
    std::vector<std::string> arguments;
    unsigned line_number = 0;

private:
    std::istream& input;
};

struct ssa_parser {
    ssa_parser(std::istream& input) : tokenizer(input) { }

    void parse(unit& u)
    {
        tokenizer.advance();
        while (accept_global(u) || accept_define(u)) { }
        expect(ssa_token_type::END);
    }

private:
    bool accept(ssa_token_type token_type)
    {
        if (tokenizer.token_type == token_type) {
            arguments = std::move(tokenizer.arguments);
            tokenizer.advance();
            return true;
        }
        return false;
    }

    void expect(ssa_token_type token_type)
    {
        if (!accept(token_type)) {
            std::stringstream message;
            message
                << "Expected '" << token_to_string.at(token_type)
                << "', got '" << token_to_string.at(tokenizer.token_type)
                << "' at line " << tokenizer.line_number;
            throw std::runtime_error(message.str());
        }
    }

    bool accept_global(unit& u)
    {
        if (!accept(ssa_token_type::GLOBAL)) {
            return false;
        }

        u.globals.insert(std::move(arguments[0]));
        return true;
    }

    bool accept_define(unit& u)
    {
        if (!accept(ssa_token_type::DEFINE)) {
            return false;
        }

        const auto& name = arguments[0];
        auto& subroutine = u.insert_subroutine(name)->second;

        auto& block = subroutine.entry_node();
        while (accept(ssa_token_type::PHI)) {
            block.instructions.emplace_back(
                instruction_type::PHI, std::move(arguments));
        }

        while (accept_instruction(block.instructions)) { }
        if (!accept_terminator(subroutine, block)) {
            return false;
        }

        while (accept_block(subroutine)) { }
        return true;
    }

    bool accept_block(subroutine& s)
    {
        if (!accept(ssa_token_type::BLOCK)) {
            return false;
        }

        const auto& name = arguments[0];
        auto& block = s.add_bb(name);

        while (accept(ssa_token_type::PHI)) {
            block.instructions.emplace_back(
                instruction_type::PHI, std::move(arguments));
        }

        while (accept_instruction(block.instructions)) { }
        if (!accept_terminator(s, block)) {
            return false;
        }
        return true;
    }

    bool accept_instruction(instruction_list& instructions)
    {
        if (accept(ssa_token_type::ARGUMENT)) {
            instructions.emplace_back(
                instruction_type::ARGUMENT, std::move(arguments));
        } else if (accept(ssa_token_type::CALL)) {
            instructions.emplace_back(
                instruction_type::CALL, std::move(arguments));
        } else if (accept(ssa_token_type::LOAD)) {
            instructions.emplace_back(
                instruction_type::LOAD, std::move(arguments));
        } else if (accept(ssa_token_type::STORE)) {
            instructions.emplace_back(
                instruction_type::STORE, std::move(arguments));
        } else if (accept(ssa_token_type::MOV)) {
            instructions.emplace_back(
                instruction_type::MOV, std::move(arguments));
        } else if (accept(ssa_token_type::ADD)) {
            instructions.emplace_back(
                instruction_type::ADD, std::move(arguments));
        } else if (accept(ssa_token_type::SUB)) {
            instructions.emplace_back(
                instruction_type::SUB, std::move(arguments));
        } else if (accept(ssa_token_type::AND)) {
            instructions.emplace_back(
                instruction_type::AND, std::move(arguments));
        } else if (accept(ssa_token_type::OR)) {
            instructions.emplace_back(
                instruction_type::OR, std::move(arguments));
        } else if (accept(ssa_token_type::LT)) {
            instructions.emplace_back(
                instruction_type::LT, std::move(arguments));
        } else if (accept(ssa_token_type::GT)) {
            instructions.emplace_back(
                instruction_type::GT, std::move(arguments));
        } else if (accept(ssa_token_type::EQ)) {
            instructions.emplace_back(
                instruction_type::EQ, std::move(arguments));
        } else if (accept(ssa_token_type::NEG)) {
            instructions.emplace_back(
                instruction_type::NEG, std::move(arguments));
        } else if (accept(ssa_token_type::NOT)) {
            instructions.emplace_back(
                instruction_type::NOT, std::move(arguments));
        } else {
            return false;
        }
        return true;
    }

    bool accept_terminator(subroutine& s, basic_block& block)
    {
        if (accept(ssa_token_type::BRANCH)) {
            s.add_cfg_edge(block, s.add_bb(arguments[1]));
            s.add_cfg_edge(block, s.add_bb(arguments[2]));
            block.instructions.emplace_back(
                instruction_type::BRANCH, std::move(arguments));
        } else if (accept(ssa_token_type::JUMP)) {
            s.add_cfg_edge(block, s.add_bb(arguments[0]));
            block.instructions.emplace_back(
                instruction_type::JUMP, std::move(arguments));
        } else if (accept(ssa_token_type::RETURN)) {
            s.add_cfg_edge(block, s.exit_node());
            block.instructions.emplace_back(
                instruction_type::RETURN, std::move(arguments));
        } else {
            return false;
        }
        return true;
    }

    ssa_tokenizer tokenizer;
    std::vector<std::string> arguments;
};

} // end anonymous namespace

namespace hcc { namespace ssa {

void unit::load(std::istream& input)
{ ssa_parser(input).parse(*this); }

}} // end namespace hcc::ssa
