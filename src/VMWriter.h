// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#pragma once
#include "ParserVM.h"
#include "asm.h"
#include <list>
#include <string>

namespace hcc {

class VMWriter
{
	int compareCounter, returnCounter;
	std::string filename, function;
	std::list<int> argStubs;
	asm_program &out;

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
	void writeLabel(const std::string label);
	void writeGoto(const std::string label);
	void writeIf(bool in, bool fin, CompareOperation op, const std::string label, bool compare, bool useConst, int intConst);
	void writeFunction(const std::string name, int argc);
	void writeCall(const std::string name, int argc);
	void writeReturn();
public:
	VMWriter(asm_program& out);
	virtual ~VMWriter();

	void setFilename(std::string& filename) {
		this->filename.assign(filename);
	}
	void writeBootstrap();
	void write(const VMCommand &c);

};

} // end namespace
