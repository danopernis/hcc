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
#include "CPU.h"
#include "instruction.h"

namespace hcc {

using namespace instruction;

void CPU::comp(unsigned short instruction, unsigned short x, unsigned short y, unsigned short &out, bool &zr, bool &ng)
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
	ng = (out & (1<<15));
}

bool CPU::jump(unsigned short instruction, bool zr, bool ng)
{
	return (((instruction & JUMP_NEG)  && ng) ||
	        ((instruction & JUMP_ZERO) && zr) ||
	        ((instruction & JUMP_POS)  && !ng && !zr));
}

void CPU::step(IROM *rom, IRAM *ram)
{
	auto olda = a;
	auto instruction = rom->get(pc);

	if (instruction & COMPUTE) {
		// COMP
		unsigned short out;
		bool zr, ng;
		CPU::comp(instruction, d, (instruction & FETCH) ? ram->get(olda) : a, out, zr, ng);

		// DEST
		if (instruction & DEST_A) {
			a = out;
		}
		if (instruction & DEST_D) {
			d = out;
		}
		if (instruction & DEST_M) {
			ram->set(olda, out);
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

} // end namespace
