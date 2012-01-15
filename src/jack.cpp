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
#include <iostream>
#include "JackTokenizer.h"
#include "JackParser.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Missing input file(s)" << std::endl;
		return 1;
	}

	for (int i = 1; i<argc; ++i) {
		std::string input(argv[i]);
		std::cout << "Processing " << input << std::endl;

		hcc::JackTokenizer tokenizer(input);
		hcc::JackParser parser(tokenizer);

		parser.parse();

		std::cout << "Class name: " << parser.c.name << std::endl;
		std::cout << "Class variables:" << std::endl;
		for (hcc::VariableList::iterator i = parser.c.variables.begin(); i != parser.c.variables.end(); ++i) {
			std::cout << "\tname: " << i->name << std::endl;
		}
		std::cout << "Class subroutines:" << std::endl;
		for (hcc::SubroutineList::iterator i = parser.c.subroutines.begin(); i != parser.c.subroutines.end(); ++i) {
			std::cout << "\tname: " << i->name << std::endl;
			std::cout << "\tlocal variables:" << std::endl;
			for (hcc::VariableList::iterator j = i->localVars.begin(); j != i->localVars.end(); ++j) {
				std::cout << "\t\tname: " << j->name << std::endl;
			}
			std::cout << std::endl;
		}
	}

	return 0;
}

