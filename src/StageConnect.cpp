// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#include "StageConnect.h"
#include "instruction.h"
#include <cstdlib>
#include <stdexcept>

namespace hcc {

using namespace instruction;

/*
 * File output
 */
VMFileOutput::VMFileOutput(const char *file)
	: stream(file)
{
}
void VMFileOutput::emitA(const std::string symbol) {
	stream << '@' << symbol << '\n';
}
void VMFileOutput::emitA(unsigned short constant) {
	if (constant & COMPUTE) {
		stream << '@' << abs((signed short)constant) << '\n';
		emitC(DEST_A | COMP_MINUS_A);
	} else {
		stream << '@' << constant << '\n';
	}
}

void VMFileOutput::emitC(unsigned short instr) {
	stream << instructionToString(instr | COMPUTE) << '\n';
}
void VMFileOutput::emitL(const std::string label)
{
	stream << '(' << label << ")\n";
}
/*
 * Asm output
 */
VMAsmOutput::~VMAsmOutput()
{
}
void VMAsmOutput::emitA(const std::string symbol) {
	AsmCommand c;
	c.type = AsmCommand::LOAD;
	c.symbol = symbol;
	asmCommands.push_back(c);
}
void VMAsmOutput::emitA(unsigned short constant) {
	AsmCommand c;
	c.type = AsmCommand::VERBATIM;

	if (constant & COMPUTE) {
		c.instr = std::abs((signed short)constant);
		asmCommands.push_back(c);
		emitC(DEST_A | COMP_MINUS_A);
	} else {
		c.instr = constant;
		asmCommands.push_back(c);
	}
}
void VMAsmOutput::emitC(unsigned short instr)
{
	AsmCommand c;
	c.type = AsmCommand::VERBATIM;
	c.instr = instr | instruction::RESERVED | instruction::COMPUTE;
	asmCommands.push_back(c);
}
void VMAsmOutput::emitL(const std::string label)
{
	AsmCommand c;
	c.type = AsmCommand::LABEL;
	c.symbol = label;
	asmCommands.push_back(c);
}

} // end namespace
