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
#include "VMParser.h"
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

#define OPTIMIZE
#if defined OPTIMIZE
		// order is somewhat important!
		// optimizations removing commands are best taken early
		hcc::optimize3(cmds, hcc::o_bloated_goto);
		hcc::optimize2(cmds, hcc::o_double_notneg);
		hcc::optimize2(cmds, hcc::o_negated_compare);
		hcc::optimize2(cmds, hcc::o_negated_if); // take this *after* o_bloated_goto

		// const expressions
		hcc::optimize3(cmds, hcc::o_const_expression3);
		hcc::optimize2(cmds, hcc::o_const_expression2);
		hcc::optimize2(cmds, hcc::o_const_if);

		// convert binary operation to unary
		hcc::optimize3(cmds, hcc::o_const_swap);
		hcc::optimize2(cmds, hcc::o_binary_to_unary);
		hcc::optimize1(cmds, hcc::o_special_unary);
		hcc::optimize3(cmds, hcc::o_binary_equalarg);

		// merge
		hcc::optimize2(cmds, hcc::o_push_pop);
		hcc::optimize2(cmds, hcc::o_compare_if);
		hcc::optimize2(cmds, hcc::o_goto_goto);
		hcc::optimize2(cmds, hcc::o_pop_push);

		// stack-less computation chain -- do NOT change order!
		hcc::optimize1(cmds, hcc::s_replicate);
		hcc::optimize2(cmds, hcc::s_reduce);
		hcc::optimize2(cmds, hcc::s_reconstruct);
#endif

		for (hcc::VMCommandList::iterator i = cmds.begin(); i != cmds.end(); ++i) {
			output.stream << (*i);
			switch (i->type) {
			case hcc::VMCommand::CONSTANT:
				writer.writeConstant(i->in, i->fin, i->int1);
				break;
			case hcc::VMCommand::PUSH:
				writer.writePush(i->in, i->fin, i->segment1, i->int1);
				break;
			case hcc::VMCommand::POP_DIRECT:
				writer.writePopDirect(i->in, i->fin, i->segment1, i->int1);
				break;
			case hcc::VMCommand::POP_INDIRECT:
				writer.writePopIndirect(i->segment1, i->int1);
				break;
			case hcc::VMCommand::POP_INDIRECT_PUSH:
				writer.writePopIndirectPush(i->in, i->fin, i->segment1, i->int1);
				break;
			case hcc::VMCommand::COPY:
				writer.writeCopy(i->segment1, i->int1, i->segment2, i->int2);
				break;
			case hcc::VMCommand::UNARY:
				writer.writeUnary(i->in, i->fin, i->unary, i->int1);
				break;
			case hcc::VMCommand::BINARY:
				writer.writeBinary(i->in, i->fin, i->binary);
				break;
			case hcc::VMCommand::COMPARE:
				writer.writeCompare(i->in, i->fin, i->compare);
				break;
			case hcc::VMCommand::UNARY_COMPARE:
				writer.writeUnaryCompare(i->in, i->fin, i->compare, i->int1);
				break;
			case hcc::VMCommand::LABEL:
				writer.writeLabel(i->arg1);
				break;
			case hcc::VMCommand::GOTO:
				writer.writeGoto(i->arg1);
				break;
			case hcc::VMCommand::IF:
				writer.writeIf(i->in, i->fin, i->compare, i->arg1, false, false, 0);
				break;
			case hcc::VMCommand::COMPARE_IF:
				writer.writeIf(i->in, i->fin, i->compare, i->arg1, true, false, 0);
				break;
			case hcc::VMCommand::UNARY_COMPARE_IF:
				writer.writeIf(i->in, i->fin, i->compare, i->arg1, true, true, i->int1);
				break;
			case hcc::VMCommand::FUNCTION:
				writer.writeFunction(i->arg1, i->int1);
				break;
			case hcc::VMCommand::CALL:
				writer.writeCall(i->arg1, i->int1);
				break;
			case hcc::VMCommand::RETURN:
				writer.writeReturn();
				break;
			case hcc::VMCommand::NOP:
			case hcc::VMCommand::IN:
			case hcc::VMCommand::FIN:
				throw std::runtime_error("helper commands made it to the final version");
				break;
			}
		}
	}
	hcc::o_stat_print();

	return 0;
}

