// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#pragma once

#include "hcc/cpu/cpu.h"
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
    cpu::word instr;
};

inline bool operator==(const instruction& a, const instruction& b)
{
    return std::tie(a.type, a.symbol, a.instr) ==
           std::tie(b.type, b.symbol, b.instr);
}

struct program {
    program() = default;
    program(std::istream&);

    void emitLoadSymbolic(std::string symbol);
    void emitLoadConstant(cpu::word constant);
    void emitInstruction(cpu::word instruction);
    void emitLabel(std::string label);
    void emitComment(std::string comment);

    void local_optimization();

    std::vector<cpu::word> assemble() const;

    void save(const std::string& filename) const;

private:
    std::vector<instruction> instructions;
};

void saveHACK(const std::string& filename, std::vector<uint16_t>);

} // namespace assembler {
} // namespace hcc {
