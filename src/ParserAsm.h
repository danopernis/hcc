// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#pragma once
#include <string>
#include <map>
#include <fstream>
#include "Assembler.h"

namespace hcc {

class ParserAsm {
private:
	std::ifstream input;
	std::string line;
	AsmCommand command;

	std::map<std::string, unsigned short> destMap, compMap, jumpMap;

public:
	ParserAsm(std::string& fileName);
	virtual ~ParserAsm();

	bool hasMoreCommands();
	void advance();

	AsmCommand getCommand() {
		return command;
	}
};

} // end namespace
