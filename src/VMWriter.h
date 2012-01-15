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
#pragma once
#include "VMParser.h"
#include "StageConnect.h"
#include <list>
#include <string>

namespace hcc {

class VMWriter
{
	int compareCounter, returnCounter;
	std::string filename;
	StringID function;
	std::list<int> argStubs;
	VMOutput &out;

	void push();
	void pop();
	void poptop(bool in);
	void load(unsigned short dest, Segment segment, unsigned int inc);
	void unaryCompare(int intArg);
	void compareBranches(bool fin, CompareOperation op);
	void push_load(Segment segment, int index);
	void writePush(bool in, bool fin, Segment segment, int index);
	void writeConstant(bool in, bool fin, int index);
	void writePopDirect(bool in, bool fin, Segment segment, int index);
	void writePopIndirect(Segment segment, int index);
	void writePopIndirectPush(bool in, bool fin, Segment segment, int index);
	void writeCopy(Segment sseg, int sind, Segment dseg, int dind);
	void writeUnary(bool in, bool fin, UnaryOperation op, int intArg);
	void writeBinary(bool in, bool fin, BinaryOperation op);
	void writeCompare(bool in, bool fin, CompareOperation op);
	void writeUnaryCompare(bool in, bool fin, CompareOperation op, int intArg);
	void writeLabel(StringID &label);
	void writeGoto(StringID &label);
	void writeIf(bool in, bool fin, CompareOperation op, StringID &label, bool compare, bool useConst, int intConst);
	void writeFunction(StringID &name, int argc);
	void writeCall(StringID &name, int argc);
	void writeReturn();
public:
	VMWriter(VMOutput& out);

	void setFilename(std::string& filename) {
		this->filename.assign(filename);
	}
	void writeBootstrap();
	void write(VMCommand &c);

};

} // end namespace
