// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#pragma once
#include <list>
#include <string>

namespace hcc {

struct AsmCommand {
	typedef enum {LOAD, VERBATIM, LABEL} Type;

	Type type;
	std::string symbol;
	unsigned short instr;
};
typedef std::list<AsmCommand> AsmCommandList;

void assemble(AsmCommandList &commands);
void outputHACK(AsmCommandList &commands, const std::string filename);

} // end namespace
