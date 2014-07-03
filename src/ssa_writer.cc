// Copyright (c) 2013-2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <boost/algorithm/string/join.hpp>

namespace hcc { namespace ssa {

const std::map<instruction_type, std::string> type_to_string = {
    { instruction_type::ARGUMENT,   "argument " },
    { instruction_type::BRANCH,     "branch " },
    { instruction_type::JUMP,       "jump " },
    { instruction_type::CALL,       "call " },
    { instruction_type::RETURN,     "return " },
    { instruction_type::LOAD,       "load " },
    { instruction_type::STORE,      "store " },
    { instruction_type::MOV,        "mov " },
    { instruction_type::ADD,        "add " },
    { instruction_type::SUB,        "sub " },
    { instruction_type::AND,        "and " },
    { instruction_type::OR,         "or " },
    { instruction_type::LT,         "lt " },
    { instruction_type::GT,         "gt " },
    { instruction_type::EQ,         "eq " },
    { instruction_type::NEG,        "neg " },
    { instruction_type::NOT,        "not " },
    { instruction_type::PHI,        "phi " },
    { instruction_type::LABEL,      "block " }
};

void unit::save(std::ostream& output) const
{
    for (const auto& global : globals) {
        output << "global " << global << "\n";
    }
    for (const auto& subroutine : subroutines) {
        output << "define " << subroutine.first << "\n";
        for (const auto& instr : subroutine.second.instructions) {
            if (instr.type != instruction_type::LABEL)
                output << "\t";

            output << instr << "\n";
        }
    }
}

std::ostream& operator<<(std::ostream& os, const instruction& instr)
{
    os << type_to_string.at(instr.type) << boost::algorithm::join(instr.arguments, " ");
    return os;
}

}} // end namespace hcc::ssa
