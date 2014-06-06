// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include "instruction.h"

namespace hcc {

enum class asm_instruction_type {
    LOAD,
    VERBATIM,
    LABEL,
    COMMENT,
};

struct asm_instruction {
    asm_instruction_type type;
    std::string symbol;
    unsigned short instr;
};

struct asm_program {
    asm_program() = default;
    asm_program(std::istream&);

    void save(const std::string& filename) const;

    std::vector<uint16_t> assemble() const;

    void emitA(const std::string symbol)
    {
        asm_instruction i;
        i.type = asm_instruction_type::LOAD;
        i.symbol = symbol;
        instructions.push_back(std::move(i));
    }
    void emitA(unsigned short constant)
    {
        asm_instruction i;
        i.type = asm_instruction_type::VERBATIM;

        if (constant & instruction::COMPUTE) {
            i.instr = std::abs((signed short)constant);
            instructions.push_back(std::move(i));
            emitC(instruction::DEST_A | instruction::COMP_MINUS_A);
        } else {
            i.instr = constant;
            instructions.push_back(std::move(i));
        }
    }
    void emitC(unsigned short instr)
    {
        asm_instruction i;
        i.type = asm_instruction_type::VERBATIM;
        i.instr = instr | instruction::RESERVED | instruction::COMPUTE;
        instructions.push_back(std::move(i));
    }
    void emitL(const std::string label)
    {
        asm_instruction i;
        i.type = asm_instruction_type::LABEL;
        i.symbol = label;
        instructions.push_back(std::move(i));
    }
    void emitComment(std::string s)
    {
        asm_instruction i;
        i.type = asm_instruction_type::COMMENT;
        i.symbol = std::move(s);
        instructions.push_back(std::move(i));
    }

private:
    std::vector<asm_instruction> instructions;
};

void saveHACK(const std::string& filename, std::vector<uint16_t>);

} // namespace hcc
