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
};

void unit::save(std::ostream& output)
{
    for (const auto& global : globals) {
        output << "global " << global << "\n";
    }
    for (auto& subroutine : subroutines) {
        output << "define " << subroutine.first << "\n";
        auto& entry_block = subroutine.second.entry_node();
        auto& exit_block = subroutine.second.exit_node();
        output << "block " << entry_block.name << "\n";
        for (const auto& instr : entry_block.instructions) {
            output << "\t" << instr << "\n";
        }
        subroutine.second.for_each_bb([&] (const basic_block& bb) {
            if (bb.index == entry_block.index ||
                bb.index == exit_block.index ||
                bb.instructions.empty())
            {
                return;
            }

            output << "block " << bb.name << "\n";
            for (const auto& instr : bb.instructions) {
                output << "\t" << instr << "\n";
            }
        });
    }
}

std::ostream& operator<<(std::ostream& os, const instruction& instr)
{
    os << type_to_string.at(instr.type) << boost::algorithm::join(instr.arguments, " ");
    return os;
}

}} // end namespace hcc::ssa
