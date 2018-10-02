// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/cpu/cpu.h"

#include "hcc/cpu/instruction.h"

namespace hcc {
namespace cpu {

using namespace instruction;

void CPU::comp(word instruction, word x, word y, word& out, bool& zr, bool& ng)
{
    if (instruction & ALU_ZX) {
        x = 0;
    }
    if (instruction & ALU_NX) {
        x = ~x;
    }
    if (instruction & ALU_ZY) {
        y = 0;
    }
    if (instruction & ALU_NY) {
        y = ~y;
    }
    if (instruction & ALU_F) {
        out = x + y;
    } else {
        out = x & y;
    }
    if (instruction & ALU_NO) {
        out = ~out;
    }
    zr = (out == 0);
    ng = (out & (1 << 15));
}

bool CPU::jump(word instruction, bool zr, bool ng)
{
    return (((instruction & JUMP_NEG) && ng) || ((instruction & JUMP_ZERO) && zr)
            || ((instruction & JUMP_POS) && !ng && !zr));
}

void CPU::step(const ROM& rom, RAM& ram)
{
    auto olda = a;
    auto instruction = rom.at(pc);

    if (instruction & COMPUTE) {
        // COMP
        word out;
        bool zr, ng;
        CPU::comp(instruction, d, (instruction & FETCH) ? ram.at(olda) : a, out, zr, ng);

        // DEST
        if (instruction & DEST_A) {
            a = out;
        }
        if (instruction & DEST_D) {
            d = out;
        }
        if (instruction & DEST_M) {
            ram.at(olda) = out;
        }

        // JUMP
        if (CPU::jump(instruction, zr, ng)) {
            pc = a;
        } else {
            ++pc;
        }
    } else {
        a = instruction;
        ++pc;
    }
}

} // namespace cpu {
} // namespace hcc {
