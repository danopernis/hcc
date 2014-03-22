// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#include <iostream>
#include <stdexcept>
#include "ParserVM.h"
#include "StageConnect.h"
#include "VMWriter.h"
#include "VMOptimize.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Missing input file(s)\n";
		return 1;
	}

	hcc::VMFileOutput output("output.asm");
	hcc::VMWriter writer(output);
	writer.writeBootstrap();

	hcc::o_stat_reset();
	for (int i = 1; i<argc; ++i) {
		std::string filename(argv[i]);
		std::cout << "***Processing " << filename << '\n';
		writer.setFilename(filename);
		hcc::VMParser parser(filename);

		hcc::VMCommandList cmds;

		// load commands from file
		while (parser.hasMoreCommands()) {
			hcc::VMCommand c = parser.advance();
			if (c.type == hcc::VMCommand::NOP)
				continue; // NOP is an artifact from parser and thus ignored

			cmds.push_back(c);
		}

		hcc::VMOptimize(cmds);

		for (hcc::VMCommandList::iterator i = cmds.begin(); i != cmds.end(); ++i) {
			output.stream << (*i);
			writer.write(*i);
		}
	}
	hcc::o_stat_print();

	return 0;
}

