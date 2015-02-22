// Copyright (c) 2013-2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include "ssa_subroutine_builder.h"
#include <sstream>

namespace {

using hcc::ssa::instruction;
using hcc::ssa::instruction_type;
using hcc::ssa::subroutine_builder;
using hcc::ssa::unit;

enum class ssa_token_type {
    GLOBAL,
    DEFINE,
    BLOCK,
    RETURN,
    JUMP,
    JLT,
    JEQ,
    CALL,
    ARGUMENT,
    LOAD,
    STORE,
    MOV,
    ADD,
    SUB,
    AND,
    OR,
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
    { ssa_token_type::JUMP,     "jump" },
    { ssa_token_type::JLT,      "jlt" },
    { ssa_token_type::JEQ,      "jeq" },
    { ssa_token_type::CALL,     "call" },
    { ssa_token_type::ARGUMENT, "argument" },
    { ssa_token_type::LOAD,     "load" },
    { ssa_token_type::STORE,    "store" },
    { ssa_token_type::MOV,      "mov" },
    { ssa_token_type::ADD,      "add" },
    { ssa_token_type::SUB,      "sub" },
    { ssa_token_type::AND,      "and" },
    { ssa_token_type::OR,       "or" },
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
    { "jump",       ssa_token_type::JUMP },
    { "jlt",        ssa_token_type::JLT },
    { "jeq",        ssa_token_type::JEQ },
    { "call",       ssa_token_type::CALL },
    { "argument",   ssa_token_type::ARGUMENT },
    { "load",       ssa_token_type::LOAD },
    { "store",      ssa_token_type::STORE },
    { "mov",        ssa_token_type::MOV },
    { "add",        ssa_token_type::ADD },
    { "sub",        ssa_token_type::SUB },
    { "and",        ssa_token_type::AND },
    { "or",         ssa_token_type::OR },
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
        subroutine_builder builder(u.insert_subroutine(name)->second);

        if (!accept_block(builder, true)) {
            return false;
        }
        while (accept_block(builder, false)) { }
        return true;
    }

    bool accept_block(subroutine_builder& builder, bool is_entry)
    {
        if (!accept(ssa_token_type::BLOCK)) {
            return false;
        }

        const auto& name = arguments[0];
        auto bb = builder.add_bb(name, is_entry);

        while (accept(ssa_token_type::PHI)) {
            builder.add_instruction(bb, instruction(
                instruction_type::PHI, std::move(arguments)));
        }

        while (accept_instruction(builder, bb)) { }
        if (!accept_terminator(builder, bb)) {
            return false;
        }
        return true;
    }

    bool accept_instruction(subroutine_builder& builder, int bb)
    {
        if (accept(ssa_token_type::ARGUMENT)) {
            builder.add_instruction(bb, instruction(
                instruction_type::ARGUMENT, std::move(arguments)));
        } else if (accept(ssa_token_type::CALL)) {
            builder.add_instruction(bb, instruction(
                instruction_type::CALL, std::move(arguments)));
        } else if (accept(ssa_token_type::LOAD)) {
            builder.add_instruction(bb, instruction(
                instruction_type::LOAD, std::move(arguments)));
        } else if (accept(ssa_token_type::STORE)) {
            builder.add_instruction(bb, instruction(
                instruction_type::STORE, std::move(arguments)));
        } else if (accept(ssa_token_type::MOV)) {
            builder.add_instruction(bb, instruction(
                instruction_type::MOV, std::move(arguments)));
        } else if (accept(ssa_token_type::ADD)) {
            builder.add_instruction(bb, instruction(
                instruction_type::ADD, std::move(arguments)));
        } else if (accept(ssa_token_type::SUB)) {
            builder.add_instruction(bb, instruction(
                instruction_type::SUB, std::move(arguments)));
        } else if (accept(ssa_token_type::AND)) {
            builder.add_instruction(bb, instruction(
                instruction_type::AND, std::move(arguments)));
        } else if (accept(ssa_token_type::OR)) {
            builder.add_instruction(bb, instruction(
                instruction_type::OR, std::move(arguments)));
        } else if (accept(ssa_token_type::NEG)) {
            builder.add_instruction(bb, instruction(
                instruction_type::NEG, std::move(arguments)));
        } else if (accept(ssa_token_type::NOT)) {
            builder.add_instruction(bb, instruction(
                instruction_type::NOT, std::move(arguments)));
        } else {
            return false;
        }
        return true;
    }

    bool accept_terminator(subroutine_builder& builder, int bb)
    {
        if (accept(ssa_token_type::JLT)) {
            builder.add_branch(bb, instruction_type::JLT, arguments[0], arguments[1], arguments[2], arguments[3]);
        } else if (accept(ssa_token_type::JEQ)) {
            builder.add_branch(bb, instruction_type::JEQ, arguments[0], arguments[1], arguments[2], arguments[3]);
        } else if (accept(ssa_token_type::JUMP)) {
            builder.add_jump(bb, arguments[0]);
        } else if (accept(ssa_token_type::RETURN)) {
            builder.add_return(bb, arguments[0]);
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
