// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include "VMCommand.h"

namespace hcc {

class VMParser {
	std::ifstream input;
	std::stringstream line;
	VMCommand c;

public:
	VMParser(std::string& fileName);
	virtual ~VMParser();

	bool hasMoreCommands();
	VMCommand& advance();
};

} // end namespace
