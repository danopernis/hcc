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
#include "instruction.h"
#include <sstream>
#include <stdexcept>

namespace hcc {

namespace instruction {

std::string instructionToString(unsigned short instr)
{
	std::stringstream stream;

	if (instr & COMPUTE) {
		if (instr & MASK_DEST) {
			if (instr & DEST_A) {
				stream << 'A';
			}
			if (instr & DEST_M) {
				stream << 'M';
			}
			if (instr & DEST_D) {
				stream << 'D';
			}
			stream << '=';
		}

		switch (instr & MASK_COMP) {
		case COMP_ZERO:
			stream << "0";
			break;
		case COMP_ONE:
			stream << "1";
			break;
		case COMP_MINUS_ONE:
			stream << "-1";
			break;
		case COMP_D:
			stream << "D";
			break;
		case COMP_A:
			stream << "A";
			break;
		case COMP_NOT_D:
			stream << "!D";
			break;
		case COMP_NOT_A:
			stream << "!A";
			break;
		case COMP_MINUS_D:
			stream << "-D";
			break;
		case COMP_MINUS_A:
			stream << "-A";
			break;
		case COMP_D_PLUS_ONE:
			stream << "D+1";
			break;
		case COMP_A_PLUS_ONE:
			stream << "A+1";
			break;
		case COMP_D_MINUS_ONE:
			stream << "D-1";
			break;
		case COMP_A_MINUS_ONE:
			stream << "A-1";
			break;
		case COMP_D_PLUS_A:
			stream << "D+A";
			break;
		case COMP_D_MINUS_A:
			stream << "D-A";
			break;
		case COMP_A_MINUS_D:
			stream << "A-D";
			break;
		case COMP_D_AND_A:
			stream << "D&A";
			break;
		case COMP_D_OR_A:
			stream << "D|A";
			break;
		case COMP_M:
			stream << "M";
			break;
		case COMP_NOT_M:
			stream << "!M";
			break;
		case COMP_MINUS_M:
			stream << "-M";
			break;
		case COMP_M_PLUS_ONE:
			stream << "M+1";
			break;
		case COMP_M_MINUS_ONE:
			stream << "M-1";
			break;
		case COMP_D_PLUS_M:
			stream << "D+M";
			break;
		case COMP_D_MINUS_M:
			stream << "D-M";
			break;
		case COMP_M_MINUS_D:
			stream << "M-D";
			break;
		case COMP_D_AND_M:
			stream << "D&M";
			break;
		case COMP_D_OR_M:
			stream << "D|M";
			break;
		default:
			throw std::runtime_error("C-instruction uses undocumented computation");
		}

		switch (instr & MASK_JUMP) {
		case JLT:
			stream << ";JLT";
			break;
		case JGT:
			stream << ";JGT";
			break;
		case JLE:
			stream << ";JLE";
			break;
		case JGE:
			stream << ";JGE";
			break;
		case JNE:
			stream << ";JNE";
			break;
		case JEQ:
			stream << ";JEQ";
			break;
		case JMP:
			stream << ";JMP";
			break;
		default:
			/* do not jump */
			break;
		}
	} else {
		stream << '@' << instr;
	}

	std::string result;
	stream >> result;
	return result;
}

} // end namespace

} // end namespace
