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
#include <sstream>
#include <iostream>
#include "instruction.h"
#include "AsmParser.h"
#include "StringTable.h"

namespace hcc {

ParserAsm::ParserAsm(std::string &fileName)
{
	input.open(fileName.c_str());

	destMap["M"]   = instruction::DEST_M;
	destMap["D"]   = instruction::DEST_D;
	destMap["MD"]  = instruction::DEST_M | instruction::DEST_D;
	destMap["A"]   = instruction::DEST_A;
	destMap["AM"]  = instruction::DEST_A | instruction::DEST_M;
	destMap["AD"]  = instruction::DEST_A | instruction::DEST_D;
	destMap["AMD"] = instruction::DEST_A | instruction::DEST_M | instruction::DEST_D;

	jumpMap["JGT"] = instruction::JGT;
	jumpMap["JEQ"] = instruction::JEQ;
	jumpMap["JGE"] = instruction::JGE;
	jumpMap["JLT"] = instruction::JLT;
	jumpMap["JNE"] = instruction::JNE;
	jumpMap["JLE"] = instruction::JLE;
	jumpMap["JMP"] = instruction::JMP;

	compMap["0"]   = instruction::COMP_ZERO;
	compMap["1"]   = instruction::COMP_ONE;
	compMap["!D"]  = instruction::COMP_NOT_D;
	compMap["!A"]  = instruction::COMP_NOT_A;
	compMap["!M"]  = instruction::COMP_NOT_M;
	compMap["-1"]  = instruction::COMP_MINUS_ONE;
	compMap["-D"]  = instruction::COMP_MINUS_D;
	compMap["-A"]  = instruction::COMP_MINUS_A;
	compMap["-M"]  = instruction::COMP_MINUS_M;
	compMap["M"]   = instruction::COMP_M;
	compMap["M+1"] = instruction::COMP_M_PLUS_ONE;
	compMap["M-1"] = instruction::COMP_M_MINUS_ONE;
	compMap["M-D"] = instruction::COMP_M_MINUS_D;
	compMap["A"]   = instruction::COMP_A;
	compMap["A+1"] = instruction::COMP_A_PLUS_ONE;
	compMap["A-1"] = instruction::COMP_A_MINUS_ONE;
	compMap["A-D"] = instruction::COMP_A_MINUS_D;
	compMap["D"]   = instruction::COMP_D;
	compMap["D+1"] = instruction::COMP_D_PLUS_ONE;
	compMap["D+A"] = instruction::COMP_D_PLUS_A;
	compMap["D+M"] = instruction::COMP_D_PLUS_M;
	compMap["D-1"] = instruction::COMP_D_MINUS_ONE;
	compMap["D-A"] = instruction::COMP_D_MINUS_A;
	compMap["D-M"] = instruction::COMP_D_MINUS_M;
	compMap["D&A"] = instruction::COMP_D_AND_A;
	compMap["D&M"] = instruction::COMP_D_AND_M;
	compMap["D|A"] = instruction::COMP_D_OR_A;
	compMap["D|M"] = instruction::COMP_D_OR_M;
}

ParserAsm::~ParserAsm()
{
	input.close();
}

bool ParserAsm::hasMoreCommands()
{
	while (input.good()) {
		getline(input, line);

		// ignore comments, starting with //
		if (line.find("//") == 0) {
			continue;
		}

		// ignore blank lines
		std::stringstream linestream(line);
		linestream >> line;
		if (line.length() == 0) {
			continue;
		}
		if (line.length() == 1 && line.at(0) == '\r') {
			continue;
		}

		break;
	}
	return input.good();
}

void ParserAsm::advance()
{
	if (line.at(0) == '@') {
		std::string symbol = line.substr(1,line.length()-1);
		if (isdigit(symbol.at(0))) {
			command.type = AsmCommand::VERBATIM;
			std::stringstream ss(symbol);
			ss >> command.instr;
		} else {
			command.type = AsmCommand::LOAD;
			command.symbol = StringTable::id(symbol);
		}
	} else if (line.at(0) == '(') {
		command.type = AsmCommand::LABEL;
		command.symbol = StringTable::id(line.substr(1,line.length()-2));
	} else {
		command.type = AsmCommand::VERBATIM;
		// dest=comp;jump
		std::string::size_type length = line.length();
		std::string::size_type equals = line.find('=');
		std::string::size_type semicolon = line.rfind(';');

		unsigned short dest, comp, jump;

		if (equals > length || equals == 0) {
			dest = 0;
			equals = -1;
		} else {
			dest = destMap[line.substr(0, equals)];
		}
		if (semicolon > length) {
			comp = compMap[line.substr(equals+1, length-equals-1)];
			jump = 0;
		} else {
			comp = compMap[line.substr(equals+1, semicolon-equals-1)];
			jump = jumpMap[line.substr(semicolon+1, 3)];
		}

		command.instr = instruction::COMPUTE | instruction::RESERVED | comp | dest | jump;
	}
}

} // end namespace
