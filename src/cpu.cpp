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
#include "cpu.h"
#include "instruction.h"

namespace hcc {

using namespace instruction;

void CPU::comp(unsigned short instr, unsigned short x, unsigned short y, unsigned short &out, bool &zr, bool &ng)
{
		if (instr & ALU_ZX)
			x = 0;
		if (instr & ALU_NX)
			x = ~x;
		if (instr & ALU_ZY)
			y = 0;
		if (instr & ALU_NY)
			y = ~y;
		out = (instr & ALU_F) ? (x + y) : (x & y);
		if (instr & ALU_NO)
			out = ~out;
		zr = (out == 0);
		ng = (out & (1<<15));
}

bool CPU::jump(unsigned short instr, bool zr, bool ng)
{
	return (((instr & JUMP_NEG)  && ng) ||
	        ((instr & JUMP_ZERO) && zr) ||
	        ((instr & JUMP_POS)  && !ng && !zr));
}

void CPU::step()
{
	unsigned short olda = a;
	unsigned short instr = rom[pc];

	if (instr & COMPUTE) {
		// COMP
		unsigned short out;
		bool zr, ng;
		CPU::comp(instr, d, (instr & FETCH) ? ram[olda] : a, out, zr, ng);

		// DEST
		if (instr & DEST_A)
			a = out;
		if (instr & DEST_D)
			d = out;
		if (instr & DEST_M)
			ram[olda] = out;

		// JUMP
		if (CPU::jump(instr, zr, ng))
			pc = a;
		else
			++pc;
	} else {
		a = instr;
		++pc;
	}
}

bool CPU::load(std::istream &in)
{
	for (unsigned int i = 0; i<romsize; ++i) {
		rom[i] = 0;
	}

	std::string line;
	unsigned int counter = 0;
	while (in.good() && counter < romsize) {
		getline(in, line);
		if (line.size() == 0)
			continue;
		if (line.size() != 16)
			return false;

		unsigned int instr = 0;
		for (unsigned int i = 0; i<16; ++i) {
			instr <<= 1;
			switch (line[i]) {
			case '0':
				break;
			case '1':
				instr |= 1;
				break;
			default:
				return false;
			}
		}
		rom[counter++] = instr;
	}
	return true;
}

} // end namespace
