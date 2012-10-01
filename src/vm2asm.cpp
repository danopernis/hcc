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
#include <iostream>
#include <stdexcept>
#include "ParserVM.h"
#include "StageConnect.h"
#include "VMWriter.h"
#include "VMOptimize.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Missing input file(s)" << std::endl;
		return 1;
	}

	hcc::VMFileOutput output("output.asm");
	hcc::VMWriter writer(output);
	writer.writeBootstrap();

	hcc::o_stat_reset();
	for (int i = 1; i<argc; ++i) {
		std::string filename(argv[i]);
		std::cout << "***Processing " << filename << std::endl;
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

