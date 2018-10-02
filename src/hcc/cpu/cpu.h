// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#pragma once

#include <array>
#include <cstdint>

namespace hcc {
namespace cpu {

using word = std::uint16_t;

struct ROM : std::array<word, 0x8000> {
    ROM() : std::array<word, 0x8000>{} {}
};

struct RAM : std::array<word, 0x6001> {
    RAM() : std::array<word, 0x6001>{} {}
};

struct CPU {
    word pc; // program counter
    word a; // register A
    word d; // register D

    void reset();
    void step(const ROM& rom, RAM& ram);
};

void comp(word instr, word x, word y, word& out, bool& zr, bool& ng);
bool jump(word instr, bool zr, bool ng);

} // namespace cpu {
} // namespace hcc {
