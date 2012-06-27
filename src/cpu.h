/*
 * Copyright (c) 2012 Dano Pernis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

namespace hcc {

/**
 * Memory interface. As far as CPU is concerned, memory is just
 * a set of registers, indexed by address.
 */
struct IRAM {
	virtual unsigned short get(unsigned int address) const = 0;
	virtual void set(unsigned int address, unsigned short value) = 0;
};
struct IROM {
	virtual unsigned short get(unsigned int address) const = 0;
};

/**
 * CPU consists of its internal state and its hardwired functionality.
 * Some functionality is factored out into static methods for later reuse.
 */
struct CPU {
	unsigned short pc;   // program counter
	unsigned short a, d; // registers
	
	static const unsigned int romsize = 0x4000;
	static const unsigned int ramsize = 0x6000;
	
	void reset() {
		pc = 0;
		a = 0;
		d = 0;
	}
	void step(IROM *rom, IRAM *ram);
	static void comp(unsigned short instr, unsigned short x, unsigned short y, unsigned short &out, bool &zr, bool &ng);
	static bool jump(unsigned short instr, bool zr, bool ng);
};

} // end namespace
