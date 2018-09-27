// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#pragma once

namespace hcc {
namespace cpu {

/**
 * Memory interface. As far as CPU is concerned, memory is just
 * a set of registers, indexed by address.
 */
struct IRAM {
    virtual ~IRAM() = default;
    virtual unsigned short get(unsigned int address) const = 0;
    virtual void set(unsigned int address, unsigned short value) = 0;
};
struct IROM {
    virtual ~IROM() = default;
    virtual unsigned short get(unsigned int address) const = 0;
};

/**
 * CPU consists of its internal state and its hardwired functionality.
 * Some functionality is factored out into static methods for later reuse.
 */
struct CPU {
    unsigned short pc; // program counter
    unsigned short a, d; // registers

    static const unsigned int romsize = 0x4000;
    static const unsigned int ramsize = 0x6000;

    void reset()
    {
        pc = 0;
        a = 0;
        d = 0;
    }
    void step(IROM* rom, IRAM* ram);
    static void comp(unsigned short instr, unsigned short x, unsigned short y, unsigned short& out,
                     bool& zr, bool& ng);
    static bool jump(unsigned short instr, bool zr, bool ng);
};

} // namespace cpu {
} // namespace hcc {
