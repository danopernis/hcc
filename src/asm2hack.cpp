// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#include <iostream>
#include <string>
#include "Assembler.h"
#include "ParserAsm.h"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		std::cerr << "Missing input file\n";
		return 1;
	}
	std::string input(argv[1]);

	// Parse
	hcc::ParserAsm parser(input);
	hcc::AsmCommandList commands;
	while (parser.hasMoreCommands()) {
		parser.advance();
		commands.push_back(parser.getCommand());
	}

	// Assemble
	hcc::assemble(commands);

	// Output
	std::string output(input, 0, input.rfind('.'));
	output.append(".hack");
	hcc::outputHACK(commands, output);

	return 0;
}
