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
#include "VMParser.h"
#include "StringTable.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <stdexcept>

namespace hcc {

Segment segmentFromString(const std::string& segment)
{
	if (segment.compare("local") == 0) {
		return LOCAL;
	} else if (segment.compare("argument") == 0) {
		return ARGUMENT;
	} else if (segment.compare("this") == 0) {
		return THIS;
	} else if (segment.compare("that") == 0) {
		return THAT;
	} else if (segment.compare("pointer") == 0) {
		return POINTER;
	} else if (segment.compare("temp") == 0) {
		return TEMP;
	} else if (segment.compare("static") == 0) {
		return STATIC;
	}
	throw std::runtime_error("invalid segment name");
}

VMParser::VMParser(std::string &fileName)
{
	input.open(fileName.c_str());
}

VMParser::~VMParser()
{
	input.close();
}

bool VMParser::hasMoreCommands()
{
	std::string tmp;
	std::getline(input, tmp);
	line.str(tmp);
	line.clear();
	return input.good();
}

VMCommand& VMParser::advance()
{
	std::vector<std::string> words;
	std::string word;

	while (line.good()) {
		line >> word;
		if (word.find("//") == 0) {
			break;
		} else {
			words.push_back(word);
		}
	}

	if (words.empty()) {
		c.type = VMCommand::NOP;
		return c;
	}
	c.in = c.fin = true;

	if (words[0].compare("label") == 0) {
		c.type = VMCommand::LABEL;
		c.arg1 = StringTable::id(words[1]);
		return c;
	} else if (words[0].compare("goto") == 0) {
		c.type = VMCommand::GOTO;
		c.arg1 = StringTable::id(words[1]);
		return c;
	} else if (words[0].compare("if-goto") == 0) {
		c.type = VMCommand::IF;
		c.arg1 = StringTable::id(words[1]);
		c.compare.set(true, false, true);
		return c;
	} else if (words[0].compare("function") == 0) {
		c.type = VMCommand::FUNCTION;
		c.arg1 = StringTable::id(words[1]);
		c.int1 = atoi(words[2].c_str());
		return c;
	} else if (words[0].compare("call") == 0) {
		c.type = VMCommand::CALL;
		c.arg1 = StringTable::id(words[1]);
		c.int1 = atoi(words[2].c_str());
		return c;
	} else if (words[0].compare("return") == 0) {
		c.type = VMCommand::RETURN;
		return c;
	} else if (words[0].compare("push") == 0) {
		c.int1 = atoi(words[2].c_str());
		if (words[1].compare("constant") == 0) {
			c.type = VMCommand::CONSTANT;
		} else {
			c.type = VMCommand::PUSH;
			c.segment1 = segmentFromString(words[1]);
		}
		return c;
	} else if (words[0].compare("pop") == 0) {
		c.int1 = atoi(words[2].c_str());
		c.segment1 = segmentFromString(words[1]);
		switch (c.segment1) {
		case LOCAL:
		case ARGUMENT:
		case THIS:
		case THAT:
			c.type = VMCommand::POP_INDIRECT;
			break;
		case POINTER:
		case TEMP:
		case STATIC:
			c.type = VMCommand::POP_DIRECT;
			break;
		}
		return c;
	} else if (words[0].compare("add") == 0) {
		c.type = VMCommand::BINARY;
		c.binary = ADD;
		return c;
	} else if (words[0].compare("sub") == 0) {
		c.type = VMCommand::BINARY;
		c.binary = SUB;
		return c;
	} else if (words[0].compare("neg") == 0) {
		c.type = VMCommand::UNARY;
		c.unary = NEG;
		return c;
	} else if (words[0].compare("and") == 0) {
		c.type = VMCommand::BINARY;
		c.binary = AND;
		return c;
	} else if (words[0].compare("or") == 0) {
		c.type = VMCommand::BINARY;
		c.binary = OR;
		return c;
	} else if (words[0].compare("not") == 0) {
		c.type = VMCommand::UNARY;
		c.unary = NOT;
		return c;
	} else if (words[0].compare("lt") == 0) {
		c.type = VMCommand::COMPARE;
		c.compare.set(true, false, false);
		return c;
	} else if (words[0].compare("eq") == 0) {
		c.type = VMCommand::COMPARE;
		c.compare.set(false, true, false);
		return c;
	} else if (words[0].compare("gt") == 0) {
		c.type = VMCommand::COMPARE;
		c.compare.set(false, false, true);
		return c;
	}
	c.type = VMCommand::NOP;
	return c;
}

} // end namespace
