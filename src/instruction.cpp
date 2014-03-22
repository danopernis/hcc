// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

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
