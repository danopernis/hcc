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
#include "VMCommand.h"
#include <iostream>
#include <map>

namespace hcc {

std::ostream& operator<<(std::ostream &out, VMCommand &c)
{
	std::map<Segment, std::string> segmentVMNames;
	segmentVMNames[STATIC]   = "static";
	segmentVMNames[POINTER]  = "pointer";
	segmentVMNames[TEMP]     = "temp";
	segmentVMNames[LOCAL]    = "local";
	segmentVMNames[ARGUMENT] = "argument";
	segmentVMNames[THIS]     = "this";
	segmentVMNames[THAT]     = "that";

	out << "\n";
	switch (c.type) {
	case VMCommand::CONSTANT:
		out << "//* push constant " << c.int1 << "\n";
		break;
	case VMCommand::PUSH:
		out << "//* push " << segmentVMNames[c.segment1] << " " << c.int1 << "\n";
		break;
	case VMCommand::POP_DIRECT:
	case VMCommand::POP_INDIRECT:
		out << "//* pop " << segmentVMNames[c.segment1] << " " << c.int1 << "\n";
		break;
	case VMCommand::POP_INDIRECT_PUSH:
		out <<	"//* pop "  << segmentVMNames[c.segment1] << " " << c.int1 << "\n"
			"//* push " << segmentVMNames[c.segment1] << " " << c.int1 << "\n";
		break;
	case VMCommand::COPY:
		out <<	"//* push " << segmentVMNames[c.segment1] << " " << c.int1 << "\n"
			"//* pop "  << segmentVMNames[c.segment2] << " " << c.int2 << "\n";
		break;
	case VMCommand::UNARY:
		out << "//* UNARY\n"; //TODO
		break;
	case VMCommand::BINARY:
		out << "//* BINARY\n"; //TODO
		break;
	case VMCommand::COMPARE:
		out << "//* COMPARE\n"; //TODO
		break;
	case VMCommand::UNARY_COMPARE:
		out << "//* push constant " << c.int1 << "\n"
		       "//* COMPARE\n"; //TODO
		break;
	case VMCommand::IF:
		out <<	"//* if-goto " << c.arg1 << "\n";
		break;
	case VMCommand::COMPARE_IF:
		out <<	"//* COMPARE\n" //TODO
		        "//* if-goto " << c.arg1 << "\n";
		break;
	case VMCommand::UNARY_COMPARE_IF:
		out <<	"//* push constant " << c.int1 << "\n"
			"//* COMPARE\n" //TODO
			"//* if-goto " << c.arg1 << "\n";
		break;
	case VMCommand::LABEL:
		out <<	"//* label " << c.arg1 << "\n";
		break;
	case VMCommand::GOTO:
		out <<	"//* goto " << c.arg1 << "\n";
		break;
	case VMCommand::FUNCTION:
		out <<	"//* function " << c.arg1 << " " << c.int1 << "\n";
		break;
	case VMCommand::CALL:
		out <<	"//* call " << c.arg1 << " " << c.int1 << "\n";
		break;
	case VMCommand::RETURN:
		out <<	"//* return\n";
		break;
	case VMCommand::NOP:
	case VMCommand::IN:
	case VMCommand::FIN:
		break;
	}
	return out;
}

} // end namespace
