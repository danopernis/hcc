// Copyright (c) 2013-2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <cassert>

namespace hcc { namespace ssa {

void instruction::use_apply(std::function<void(std::string&)> g)
{
    auto f = [&] (std::string& s) {
        if (!s.empty() && s[0] == '%')
            g(s);
    };

    switch (type) {
    case instruction_type::CALL: {
        int counter = 0;
        for (auto& argument : arguments) {
            if (counter >= 2) {
                f(argument);
            }
            ++counter;
        }
        } break;
    case instruction_type::PHI: {
        int counter = 0;
        for (auto& argument : arguments) {
            if (counter != 0 && counter % 2 == 0) {
                f(argument);
            }
            ++counter;
        }
        } break;
    case instruction_type::RETURN:
    case instruction_type::BRANCH:
        f(arguments[0]);
        break;
    case instruction_type::LABEL:
    case instruction_type::JUMP:
    case instruction_type::ARGUMENT:
        break;
    case instruction_type::STORE:
        f(arguments[0]);
        f(arguments[1]);
        break;
    case instruction_type::ADD:
    case instruction_type::SUB:
    case instruction_type::AND:
    case instruction_type::OR:
    case instruction_type::LT:
    case instruction_type::GT:
    case instruction_type::EQ:
        f(arguments[1]);
        f(arguments[2]);
        break;
    case instruction_type::LOAD:
    case instruction_type::MOV:
    case instruction_type::NEG:
    case instruction_type::NOT:
        f(arguments[1]);
        break;
    default:
        assert(false);
    }
}

void instruction::def_apply(std::function<void(std::string&)> f)
{
    switch (type) {
    case instruction_type::BRANCH:
    case instruction_type::JUMP:
    case instruction_type::RETURN:
    case instruction_type::LABEL:
    case instruction_type::STORE:
        break;
    case instruction_type::CALL:
    case instruction_type::LOAD:
    case instruction_type::MOV:
    case instruction_type::NEG:
    case instruction_type::NOT:
    case instruction_type::ADD:
    case instruction_type::SUB:
    case instruction_type::AND:
    case instruction_type::OR:
    case instruction_type::LT:
    case instruction_type::GT:
    case instruction_type::EQ:
    case instruction_type::PHI:
    case instruction_type::ARGUMENT:
        f(arguments[0]);
        break;
    default:
        assert(false);
    }
}

void instruction::label_apply(std::function<void(std::string&)> f)
{
    switch (type) {
    case instruction_type::LABEL:
    case instruction_type::JUMP:
        f(arguments[0]);
        break;
    case instruction_type::BRANCH:
        f(arguments[1]);
        f(arguments[2]);
        break;
    case instruction_type::PHI: {
        int counter = 0;
        for (auto& argument : arguments) {
            if (counter % 2 == 1) {
                f(argument);
            }
            ++counter;
        }
        } break;
    case instruction_type::RETURN:
    case instruction_type::STORE:
    case instruction_type::CALL:
    case instruction_type::LOAD:
    case instruction_type::MOV:
    case instruction_type::NEG:
    case instruction_type::NOT:
    case instruction_type::ADD:
    case instruction_type::SUB:
    case instruction_type::AND:
    case instruction_type::OR:
    case instruction_type::LT:
    case instruction_type::GT:
    case instruction_type::EQ:
    case instruction_type::ARGUMENT:
        break;
    default:
        assert(false);
    }
}

}} // end namespace hcc::ssa
