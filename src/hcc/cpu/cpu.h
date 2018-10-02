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

/**
 * CPU consists of its internal state and its hardwired functionality.
 * Some functionality is factored out into static methods for later reuse.
 */
struct CPU {
    word pc; // program counter
    word a; // register A
    word d; // register D

    void reset()
    {
        pc = 0;
        a = 0;
        d = 0;
    }
    void step(const ROM& rom, RAM& ram);
    static void comp(word instr, word x, word y, word& out, bool& zr, bool& ng);
    static bool jump(word instr, bool zr, bool ng);
};

} // namespace cpu {
} // namespace hcc {
