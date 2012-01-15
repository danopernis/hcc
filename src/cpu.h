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
#include <iostream>

namespace hcc {

class CPU {
	unsigned short pc, a, d, *rom, *ram; // program counter, registers, memory
public:
	static const unsigned short romsize = 0x4000;
	static const unsigned short ramsize = 0x6000;

	CPU() {
		rom = new unsigned short[romsize];
		ram = new unsigned short[ramsize];
		reset();
	}
	virtual ~CPU() {
		delete[] rom;
		delete[] ram;
	}
	static void comp(unsigned short instr, unsigned short x, unsigned short y, unsigned short &out, bool &zr, bool &ng);
	static bool jump(unsigned short instr, bool zr, bool ng);
	void reset() {
		pc = 0;
	}
	unsigned short getRam(unsigned short address) {
		return ram[address];
	}
	void step();
	bool load(std::istream &in);
};

} // end namespace
