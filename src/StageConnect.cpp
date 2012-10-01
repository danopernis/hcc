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
	stream << instructionToString(instr | COMPUTE) << std::endl;
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
