// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

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
		outputFile << '\n';
	}
}

} //end namespace
