// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include "ssa_tokenizer.h"
#include "ssa_subroutine_builder.h"
#include <sstream>

namespace {

using hcc::ssa::argument;
using hcc::ssa::constant;
using hcc::ssa::global;
using hcc::ssa::instruction;
using hcc::ssa::instruction_type;
using hcc::ssa::label;
using hcc::ssa::subroutine_builder;
using hcc::ssa::unit;
using ssa_tokenizer = hcc::ssa::tokenizer;
using ssa_token_type = hcc::ssa::token_type;

struct ssa_parser {
    ssa_parser(std::istream& input)
        : tokenizer(input)
    {
    }

    void parse(unit& u)
    {
        current = tokenizer.next();
        while (accept_define(u)) {
        }
        expect(ssa_token_type::END);
    }

private:
    bool accept(ssa_token_type token_type)
    {
        if (current.type == token_type) {
            token = current.token;
            current = tokenizer.next();
            return true;
        }
        return false;
    }

    void expect(ssa_token_type token_type)
    {
        if (!accept(token_type)) {
            std::stringstream message;
            message << "Expected '" << to_string(token_type) << "', got '"
                    << to_string(current.type) << "' at line " << current.line_number;
            throw std::runtime_error(message.str());
        }
    }

    argument expect_register(subroutine_builder& builder)
    {
        expect(ssa_token_type::REG);
        return builder.add_reg(token);
    }

    argument expect_constant()
    {
        expect(ssa_token_type::CONSTANT);
        return constant(std::stoi(token));
    }

    global expect_global(unit& u)
    {
        expect(ssa_token_type::GLOBAL);
        return u.globals.put(token);
    }

    argument expect_value(unit& u, subroutine_builder& builder)
    {
        if (accept(ssa_token_type::CONSTANT)) {
            return constant(std::stoi(token));
        } else if (accept(ssa_token_type::REG)) {
            return builder.add_reg(token);
        } else if (accept(ssa_token_type::LOCAL)) {
            return builder.add_local(token);
        } else {
            return expect_global(u);
        }
    }

    bool accept_define(unit& u)
    {
        if (!accept(ssa_token_type::DEFINE)) {
            return false;
        }

        const auto& name = expect_global(u);
        auto builder = subroutine_builder{u.insert_subroutine(name)->second};

        expect_block(u, builder, true);
        while (accept_block(u, builder, false)) {
        }

        return true;
    }

    void expect_block(unit& u, subroutine_builder& builder, bool is_entry)
    {
        if (!accept_block(u, builder, is_entry)) {
            std::stringstream message;
            message << "Expected block at line " << current.line_number;
            throw std::runtime_error(message.str());
        }
    }

    bool accept_block(unit& u, subroutine_builder& builder, bool is_entry)
    {
        if (!accept(ssa_token_type::BLOCK)) {
            return false;
        }

        auto bb = expect_label(builder, is_entry);

        while (accept(ssa_token_type::PHI)) {
            instruction i(instruction_type::PHI, {});
            i.arguments.push_back(expect_register(builder));
            while (!accept(ssa_token_type::SEMICOLON)) {
                i.arguments.emplace_back(expect_label(builder));
                i.arguments.push_back(expect_value(u, builder));
            }
            builder.add_instruction(bb, std::move(i));
        }

        while (accept_instruction(u, builder, bb)) {
        }
        if (!accept_terminator(u, builder, bb)) {
            return false;
        }
        return true;
    }

    bool accept_instruction(unit& u, subroutine_builder& builder, const label& bb)
    {
        if (accept(ssa_token_type::ARGUMENT)) {
            const auto dest = expect_register(builder);
            const auto index = expect_constant();
            expect(ssa_token_type::SEMICOLON);

            builder.add_instruction(bb, instruction(instruction_type::ARGUMENT, {dest, index}));
        } else if (accept(ssa_token_type::CALL)) {
            const auto dest = expect_register(builder);
            const auto func = expect_global(u);
            std::vector<argument> arguments{dest, func};
            while (!accept(ssa_token_type::SEMICOLON)) {
                arguments.push_back(expect_value(u, builder));
            }
            builder.add_instruction(bb, instruction(instruction_type::CALL, std::move(arguments)));
        } else if (accept(ssa_token_type::STORE)) {
            const auto dest = expect_value(u, builder);
            const auto src = expect_value(u, builder);
            expect(ssa_token_type::SEMICOLON);

            builder.add_instruction(bb, instruction(instruction_type::STORE, {dest, src}));
        } else if (accept(ssa_token_type::LOAD)) {
            expect_unary_instruction(u, builder, bb, instruction_type::LOAD);
        } else if (accept(ssa_token_type::MOV)) {
            expect_unary_instruction(u, builder, bb, instruction_type::MOV);
        } else if (accept(ssa_token_type::NEG)) {
            expect_unary_instruction(u, builder, bb, instruction_type::NEG);
        } else if (accept(ssa_token_type::NOT)) {
            expect_unary_instruction(u, builder, bb, instruction_type::NOT);
        } else if (accept(ssa_token_type::ADD)) {
            expect_binary_instruction(u, builder, bb, instruction_type::ADD);
        } else if (accept(ssa_token_type::SUB)) {
            expect_binary_instruction(u, builder, bb, instruction_type::SUB);
        } else if (accept(ssa_token_type::AND)) {
            expect_binary_instruction(u, builder, bb, instruction_type::AND);
        } else if (accept(ssa_token_type::OR)) {
            expect_binary_instruction(u, builder, bb, instruction_type::OR);
        } else {
            return false;
        }
        return true;
    }

    void expect_binary_instruction(unit& u, subroutine_builder& builder, const label& bb,
                                   const instruction_type& type)
    {
        const auto dest = expect_register(builder);
        const auto arg1 = expect_value(u, builder);
        const auto arg2 = expect_value(u, builder);
        expect(ssa_token_type::SEMICOLON);

        builder.add_instruction(bb, instruction(type, {dest, arg1, arg2}));
    }

    void expect_unary_instruction(unit& u, subroutine_builder& builder, const label& bb,
                                  const instruction_type& type)
    {
        const auto dest = expect_register(builder);
        const auto arg = expect_value(u, builder);
        expect(ssa_token_type::SEMICOLON);

        builder.add_instruction(bb, instruction(type, {dest, arg}));
    }

    bool accept_terminator(unit& u, subroutine_builder& builder, const label& bb)
    {
        if (accept(ssa_token_type::JLT)) {
            expect_branch(u, builder, bb, instruction_type::JLT);
        } else if (accept(ssa_token_type::JEQ)) {
            expect_branch(u, builder, bb, instruction_type::JEQ);
        } else if (accept(ssa_token_type::JUMP)) {
            const auto target = expect_label(builder);
            builder.add_jump(bb, target);
        } else if (accept(ssa_token_type::RETURN)) {
            const auto result = expect_value(u, builder);
            builder.add_return(bb, result);
        } else {
            return false;
        }
        expect(ssa_token_type::SEMICOLON);
        return true;
    }

    void expect_branch(unit& u, subroutine_builder& builder, const label& bb,
                       const instruction_type& type)
    {
        const auto arg1 = expect_value(u, builder);
        const auto arg2 = expect_value(u, builder);
        const auto positive = expect_label(builder);
        const auto negative = expect_label(builder);
        builder.add_branch(bb, type, arg1, arg2, positive, negative);
    }

    label expect_label(subroutine_builder& builder, bool is_entry = false)
    {
        expect(ssa_token_type::LABEL);
        return builder.add_bb(token, is_entry);
    }

    ssa_tokenizer tokenizer;
    std::string token;
    hcc::ssa::token current;
};

} // namespace {

namespace hcc {
namespace ssa {

void unit::load(std::istream& input) { ssa_parser(input).parse(*this); }

} // namespace ssa {
} // namespace hcc {
