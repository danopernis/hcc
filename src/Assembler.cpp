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
#include <fstream>
#include <sstream>
#include <map>
#include "Assembler.h"

namespace hcc {

void assemble(AsmCommandList &commands)
{
	// Built-in symbols
	std::map<std::string, int> table;
	table["SP"]   = 0x0000;
	table["LCL"]  = 0x0001;
	table["ARG"]  = 0x0002;
	table["THIS"] = 0x0003;
	table["THAT"] = 0x0004;
	for (unsigned int i = 0; i<16; ++i) {
		std::stringstream name;
		name << "R" << i;
		table[name.str()] = i;
	}
	table["SCREEN"] = 0x4000;
	table["KBD"]    = 0x6000;

	// First pass. Assign addresses to label symbols
	unsigned int address = 0;
	for (hcc::AsmCommandList::iterator i = commands.begin(), e = commands.end(); i!=e; ++i) {
		switch (i->type) {
		case hcc::AsmCommand::LABEL:
			table[i->symbol] = address;
			break;
		case hcc::AsmCommand::LOAD:
		case hcc::AsmCommand::VERBATIM:
			++address;
			break;
		}
	}

	// Second pass
	unsigned int variable = 0x10;
	for (hcc::AsmCommandList::iterator i = commands.begin(), e = commands.end(); i!=e; ++i) {
		if (i->type != hcc::AsmCommand::LOAD)
			continue;

		if (table.find(i->symbol) == table.end()) {
			table[i->symbol] = variable;
			i->instr = variable;
			++variable;
		} else {
			i->instr = table[i->symbol];
		}
	}
}

void outputHACK(AsmCommandList &commands, const std::string filename)
{
	std::ofstream outputFile(filename.c_str());

	for (AsmCommandList::iterator i = commands.begin(); i!=commands.end(); ++i) {
		if (i->type == AsmCommand::LABEL)
			continue;

		for (int j = 15; j>=0; --j) {
			outputFile << (i->instr & (1<<15) ? '1' : '0');
			i->instr <<= 1;
		}
		outputFile << std::endl;
	}
}

} //end namespace
