// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#pragma once

#include "hcc/cpu/instruction.h"

#include <istream>
#include <string>
#include <tuple>
#include <vector>

namespace hcc {
namespace assembler {

enum class instruction_type {
    LOAD,
    VERBATIM,
    LABEL,
    COMMENT,
};

struct instruction {
    instruction_type type;
    std::string symbol;
    unsigned short instr;
};

inline bool operator==(const instruction& a, const instruction& b)
{
    return std::tie(a.type, a.symbol, a.instr) ==
           std::tie(b.type, b.symbol, b.instr);
}

struct program {
    program() = default;
    program(std::istream&);

    void save(const std::string& filename) const;

    std::vector<uint16_t> assemble() const;

    void emitA(const std::string symbol)
    {
        instruction i;
        i.type = instruction_type::LOAD;
        i.symbol = symbol;
        instructions.push_back(std::move(i));
    }
    void emitA(unsigned short constant)
    {
        instruction i;
        i.type = instruction_type::VERBATIM;

        if (constant & hcc::instruction::COMPUTE) {
            i.instr = std::abs((signed short)constant);
            instructions.push_back(std::move(i));
            emitC(hcc::instruction::DEST_A | hcc::instruction::COMP_MINUS_A);
        } else {
            i.instr = constant;
            instructions.push_back(std::move(i));
        }
    }
    void emitC(unsigned short instr)
    {
        instruction i;
        i.type = instruction_type::VERBATIM;
        i.instr = instr | hcc::instruction::RESERVED | hcc::instruction::COMPUTE;
        instructions.push_back(std::move(i));
    }
    void emitL(const std::string label)
    {
        instruction i;
        i.type = instruction_type::LABEL;
        i.symbol = label;
        instructions.push_back(std::move(i));
    }
    void emitComment(std::string s)
    {
        instruction i;
        i.type = instruction_type::COMMENT;
        i.symbol = std::move(s);
        instructions.push_back(std::move(i));
    }

    void local_optimization();

private:
    std::vector<instruction> instructions;
};

void saveHACK(const std::string& filename, std::vector<uint16_t>);

} // namespace assembler {
} // namespace hcc {
