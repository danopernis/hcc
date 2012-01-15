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
#include "VMWriter.h"
#include "instruction.h"
#include <cstdlib>
#include <stdexcept>

namespace hcc {

using namespace instruction;

VMWriter::VMWriter(std::string &fileName)
{
	out.open(fileName.c_str());
	compareCounter = 0;
	returnCounter = 0;
}
StringID& constructString(const char *s, unsigned int i)
{
	std::stringstream ss;
	ss << s << i;
	std::string s2 = ss.str();
	return StringTable::id(s2);
}
StringID& constructString(StringID &s1, StringID &s2)
{
	std::stringstream ss;
	ss << s1 << '$' << s2;
	std::string s3 = ss.str();
	return StringTable::id(s3);
}
void VMWriter::push()
{
	emitA	("SP");
	emitC	(DEST_M | COMP_M_PLUS_ONE);// ++SP
	emitC	(DEST_A | COMP_M_MINUS_ONE);
	emitC	(DEST_M | COMP_D);// push
}
void VMWriter::pop()
{
	emitA	("SP");
	emitC	(DEST_A | DEST_M | COMP_M_MINUS_ONE);// --SP
	emitC	(DEST_D | COMP_M);// pop
}
void VMWriter::unaryCompare(int intArg)
{
	switch(intArg) {
	case 0:
		// D=D-0
		break;
	case 1:
		emitC	(DEST_D | COMP_D_MINUS_ONE);// comparison
		break;
	case -1:
		emitC	(DEST_D | COMP_D_PLUS_ONE);// comparison
		break;
	default:
		emitA	(intArg);
		emitC	(DEST_D | COMP_D_MINUS_A);// comparison
		break;
	}
}
/*
 * reg = M[segment] + index
 */
void VMWriter::load(unsigned short dest, Segment segment, unsigned int index)
{
	// some instructions can be saved when index = 0, 1 or 2
	if (index > 2) {
		emitA	(index);
		emitC	(DEST_D | COMP_A);
	}
	switch (segment) {
	case LOCAL:
		emitA	("LCL");
		break;
	case ARGUMENT:
		emitA	("ARG");
		break;
	case THIS:
		emitA	("THIS");
		break;
	case THAT:
		emitA	("THAT");
		break;
	default:
		throw 1;
	}
	switch (index) {
	case 0:
		emitC	(dest | COMP_M);
		break;
	case 1:
		emitC	(dest | COMP_M_PLUS_ONE);
		break;
	case 2:
		emitC	(dest | COMP_M_PLUS_ONE);
		if (dest == DEST_D) {
			emitC	(DEST_D | COMP_D_PLUS_ONE);
		}
		if (dest == DEST_A) {
			emitC	(DEST_A | COMP_A_PLUS_ONE);
		}
		break;
	default:
		emitC	(dest | COMP_D_PLUS_M);
		break;
	}
}
void VMWriter::push_load(Segment segment, int index)
{
	switch (segment) {
	case STATIC:
		emitA	(constructString(filename.c_str(), index));
		emitC	(DEST_D | COMP_M);
		break;
	case POINTER:
		emitA	(3 + index);
		emitC	(DEST_D | COMP_M);
		break;
	case TEMP:
		emitA	(5 + index);
		emitC	(DEST_D | COMP_M);
		break;
	case LOCAL:
	case ARGUMENT:
	case THIS:
	case THAT:
		load(DEST_A, segment, index);
		emitC	(DEST_D | COMP_M);
		break;
	}
}
void VMWriter::poptop(bool in)
{
	if (in) {
		pop();
		emitC	(DEST_A | COMP_A_MINUS_ONE);
	} else {
		emitA	("SP");
		emitC	(DEST_A | COMP_M_MINUS_ONE);
	}
}

/*
 * CONSTANT, PUSH, PCOMP_DIRECT, PCOMP_INDIRECT, COPY
 */
void VMWriter::writeConstant(bool in, bool fin, int value)
{
	if (fin && -2 <= value && value <= 2) {
		emitA	("SP");
		emitC	(DEST_M | COMP_M_PLUS_ONE);// ++SP
		emitC	(DEST_A | COMP_M_MINUS_ONE);
		switch (value) {
		case -2:
			emitC	(DEST_M | COMP_MINUS_ONE);
			emitC	(DEST_M | COMP_M_MINUS_ONE);
			break;
		case -1:
			emitC	(DEST_M | COMP_MINUS_ONE);
			break;
		case 0:
			emitC	(DEST_M | COMP_ZERO);
			break;
		case 1:
			emitC	(DEST_M | COMP_ONE);
			break;
		case 2:
			emitC	(DEST_M | COMP_ONE);
			emitC	(DEST_M | COMP_M_PLUS_ONE);
			break;
		}
		return;
	}

	if (value == -1) {
		emitC	(DEST_D | COMP_MINUS_ONE);
	} else if (value == 0) {
		emitC	(DEST_D | COMP_ZERO);
	} else if (value == 1) {
		emitC	(DEST_D | COMP_ONE);
	} else if (value < 0) {
		emitA	(-value);
		emitC	(DEST_D | COMP_MINUS_A);
	} else {
		emitA	(value);
		emitC	(DEST_D | COMP_A);
	}
	if (fin)
		push();
}
void VMWriter::writePush(bool in, bool fin, Segment segment, int index)
{
	push_load(segment, index);
	if (fin)
		push();
}
void VMWriter::writePopDirect(bool in, bool fin, Segment segment, int index)
{
	if (in)
		pop();
	switch (segment) {
	case STATIC:
		emitA	(constructString(filename.c_str(), index));
		break;
	case POINTER:
		emitA	(3 + index);
		break;
	case TEMP:
		emitA	(5 + index);
		break;
	case LOCAL:
	case ARGUMENT:
	case THIS:
	case THAT:
		throw 1;
	}
	emitC	(DEST_M | COMP_D);// save
}
void VMWriter::writePopIndirect(Segment segment, int index)
{
	load(DEST_D, segment, index);
	emitA	("R15");
	emitC	(DEST_M | COMP_D);// save calculated address for popping
	pop();
	emitA	("R15");
	emitC	(DEST_A | COMP_M);// load the address
	emitC	(DEST_M | COMP_D);// save
}
void VMWriter::writePopIndirectPush(bool in, bool fin, Segment segment, int index)
{
	load(DEST_D, segment, index);
	emitA	("R15");
	emitC	(DEST_M | COMP_D);// save calculated address for popping
	pop();
	emitA	("R15");
	emitC	(DEST_A | COMP_M);// load the address
	emitC	(DEST_M | COMP_D);// save
	if (fin)
		push();
}
void VMWriter::writeCopy(Segment sseg, int sind, Segment dseg, int dind)
{
	if (sseg == dseg) {
		if (sind == dind)
			return; // no need to copy

		if (std::abs(dind-sind) < 4) { // 4 is empiric constant
			load(DEST_A, sseg, sind);
			emitC	(DEST_D | COMP_M);// fetch
			for (int i = sind; i>dind; --i)
				emitC	(DEST_A | COMP_A_MINUS_ONE);
			for (int i = sind; i<dind; ++i)
				emitC	(DEST_A | COMP_A_PLUS_ONE);
			emitC	(DEST_M | COMP_D);// store
			return;
		}
	}

	//TODO: do not duplicate writePop()
	load(DEST_D, dseg, dind);
	emitA	("R15");
	emitC	(DEST_M | COMP_D);// save calculated address for popping
	push_load(sseg, sind);
	emitA	("R15");
	emitC	(DEST_A | COMP_M);// load the address
	emitC	(DEST_M | COMP_D);// copy
}
/*
 * UNARY, BINARY
 */
void VMWriter::writeUnary(bool in, bool fin, UnaryOperation op, int intArg)
{
	if (in && fin) {
		std::map<UnaryOperation, unsigned short> unaryTable;
		unaryTable[NOT]  = DEST_M | COMP_NOT_M;
		unaryTable[NEG]  = DEST_M | COMP_MINUS_M;
		unaryTable[ADDC] = DEST_M | COMP_D_PLUS_M;
		unaryTable[SUBC] = DEST_M | COMP_M_MINUS_D;
		unaryTable[BUSC] = DEST_M | COMP_D_MINUS_M;
		unaryTable[ANDC] = DEST_M | COMP_D_AND_M;
		unaryTable[ORC]  = DEST_M | COMP_D_OR_M;

		switch (op) {
		case NOT:
		case NEG:
			emitA	("SP");
			emitC	(DEST_A | COMP_M_MINUS_ONE);
			emitC	(unaryTable[op]);
			break;
		case DOUBLE:
			emitC	(DEST_D | COMP_M);
			emitC	(DEST_M | COMP_D_PLUS_M);
			break;
		case SUBC:
			if (intArg == 1) {
				emitA	("SP");
				emitC	(DEST_A | COMP_M_MINUS_ONE);
				emitC	(DEST_M | COMP_M_MINUS_ONE);
				return;
			}
			// no break;
		case ADDC:
		case BUSC:
		case ANDC:
		case ORC:
			emitA	(intArg);
			emitC	(DEST_D | COMP_A);
			emitA	("SP");
			emitC	(DEST_A | COMP_M_MINUS_ONE);
			emitC	(unaryTable[op]);
			break;
		}

	} else {
		if (in)
			pop();
		switch (op) {
		case NOT:
			emitC	(DEST_D | COMP_NOT_D);
			break;
		case NEG:
			emitC	(DEST_D | COMP_MINUS_D);
			break;
		case DOUBLE:
			emitC	(DEST_A | COMP_D);
			emitC	(DEST_D | COMP_D_PLUS_A);
			break;
		case ADDC:
			if (intArg == 1)  {
				emitC	(DEST_D | COMP_D_PLUS_ONE);
			} else {
				emitA	(intArg);
				emitC	(DEST_D | COMP_D_PLUS_A);
			}
			break;
		case SUBC:
			if (intArg == 1) {
				emitC	(DEST_D | COMP_D_MINUS_ONE);
			} else {
				emitA	(intArg);
				emitC	(DEST_D | COMP_D_MINUS_A);
			}
			break;
		case BUSC:
			emitA	(intArg);
			emitC	(DEST_D | COMP_A_MINUS_D);
			break;
		case ANDC:
			emitA	(intArg);
			emitC	(DEST_D | COMP_D_AND_A);
			break;
		case ORC:
			emitA	(intArg);
			emitC	(DEST_D | COMP_D_OR_A);
			break;
		}
		if (fin)
			push();
	}
}
void VMWriter::writeBinary(bool in, bool fin, BinaryOperation op)
{
	unsigned short dest;
	if (fin) { // store result to memory; do not adjust SP
		dest = DEST_M;
		poptop(in);
	} else { // store result to register; adjust SP
		dest = DEST_D;
		if (in) // fetch argument from stack
			pop();
			emitA	("SP");
			emitC	(DEST_A | DEST_M | COMP_M_MINUS_ONE);
	}
	switch (op) {
	case ADD:
		emitC	(dest | COMP_D_PLUS_M);
		break;
	case SUB:
		emitC	(dest | COMP_M_MINUS_D);
		break;
	case BUS:
		emitC	(dest | COMP_D_MINUS_M);
		break;
	case AND:
		emitC	(dest | COMP_D_AND_M);
		break;
	case OR:
		emitC	(dest | COMP_D_OR_M);
		break;
	}
}
/*
 * UNARY_COMPARE, COMPARE
 */
void VMWriter::compareBranches(bool fin, CompareOperation op)
{
	StringID compareSwitch = constructString("__compareSwitch", compareCounter);
	StringID compareEnd    = constructString("__compareEnd", compareCounter);
	++compareCounter;

	if (fin) {
		emitA	(compareEnd);
		emitC	(COMP_D | op.jump());
		emitA	("SP");
		emitC	(DEST_A | COMP_M_MINUS_ONE);
		emitC	(DEST_M | COMP_ZERO);// adjust to false
		emitL	(compareEnd);
	} else {
		emitA	(compareSwitch);
		emitC	(COMP_D | op.jump());
		emitC	(DEST_D | COMP_ZERO);
		emitA	(compareEnd);
		emitC	(COMP_ZERO | JMP);
		emitL	(compareSwitch);
		emitC	(DEST_D | COMP_MINUS_ONE);
		emitL	(compareEnd);
	}
}
void VMWriter::writeUnaryCompare(bool in, bool fin, CompareOperation op, int intArg)
{
	if (fin) { // save result to memory
		if (in) { // fetch argument from stack, do not adjust SP
			emitA	("SP");
			emitC	(DEST_A | COMP_M_MINUS_ONE);
			emitC	(DEST_D | COMP_M);
		} else { // argument is in register, increment SP
			emitA	("SP");
			emitC	(DEST_M | COMP_M_PLUS_ONE);
			emitC	(DEST_A | COMP_M_MINUS_ONE);
		}
		emitC(DEST_M | COMP_MINUS_ONE);// default is true
	} else { // save result to register
		if (in)
			pop(); // fetch argument from stack
	}
	unaryCompare(intArg);
	compareBranches(fin, op);
}
void VMWriter::writeCompare(bool in, bool fin, CompareOperation op)
{
	poptop(in);
	emitC	(DEST_D | COMP_M_MINUS_D);// comparison
	if (fin) {
		emitC(DEST_M | COMP_MINUS_ONE);// default is true
	} else {
		emitA	("SP");
		emitC	(DEST_M | COMP_M_MINUS_ONE);
	}
	compareBranches(fin, op);
}
/*
 * LABEL, GOTO, IF, COMPARE_IF, UNARY_COMPARE_IF
 */
void VMWriter::writeLabel(StringID &label)
{
	emitL	(constructString(function, label));
}
void VMWriter::writeGoto(StringID &label)
{
	emitA	(constructString(function, label));
	emitC	(COMP_ZERO | JMP);
}
void VMWriter::writeIf(bool in, bool fin, CompareOperation op, StringID &label, bool compare, bool useConst, int intConst)
{
	if (in)
		pop();
	if (compare) {
		if (useConst) {	// UNARY_COMPARE_IF
			unaryCompare(intConst);
		} else {	// COMPARE_IF
			emitA	("SP");
			emitC	(DEST_A | DEST_M | COMP_M_MINUS_ONE);// --SP
			emitC	(DEST_D | COMP_M_MINUS_D);// comparison
		}
	} // else just IF
	emitA	(constructString(function, label));
	emitC	(COMP_D | op.jump());
}
/*
 * FUNCTION, CALL, RETURN
 */
void VMWriter::writeFunction(StringID &name, int localc)
{
	function = name;
	emitL(name);
	switch (localc) {
	case 0:
		break;
	case 1:
		emitA	("SP");
		emitC	(DEST_M | COMP_M_PLUS_ONE);
		emitC	(DEST_A | COMP_M_MINUS_ONE);
		emitC	(DEST_M | COMP_ZERO);
		break;
	default: // 2*localc+4 instructions
		emitA	("SP");
		emitC	(DEST_A | COMP_M);
		for (int i = 1; i<localc; ++i) {
			emitC	(DEST_M | COMP_ZERO);
			emitC	(DEST_A | COMP_A_PLUS_ONE);
		}
		emitC	(DEST_M | COMP_ZERO);
		emitC	(DEST_D | COMP_A_PLUS_ONE);// "unroll"
		emitA	("SP");
		emitC	(DEST_M | COMP_D);
		break;
	}
}
// 44 instructions when called for the first time
//  8 instructions when called next time with the same argc
void VMWriter::writeCall(StringID &name, int argc)
{
	bool found = false;
	for (std::list<int>::iterator i = argStubs.begin(); i != argStubs.end(); ++i) {
		if (*i == argc) {
			found = true;
		}
	}

	StringID call          = constructString("__call", argc);
	StringID returnAddress = constructString("__returnAddress", returnCounter);
	++returnCounter;

	emitA	(name);
	emitC	(DEST_D | COMP_A);
	emitA	("R15");
	emitC	(DEST_M | COMP_D);
	emitA	(returnAddress);
	emitC	(DEST_D | COMP_A);
	if (found) {
		emitA	(call);
		emitC	(COMP_ZERO | JMP);
	} else {
		emitL	(call);
		push();
		emitA	("LCL");
		emitC	(DEST_D | COMP_M);
		push();
		emitA	("ARG");
		emitC	(DEST_D | COMP_M);
		push();
		emitA	("THIS");
		emitC	(DEST_D | COMP_M);
		push();
		emitA	("THAT");
		emitC	(DEST_D | COMP_M);
		push();
		emitA	("SP");
		emitC	(DEST_D | COMP_M);
		emitA	("LCL");
		emitC	(DEST_M | COMP_D);// LCL = SP
		emitA	(argc + 5);
		emitC	(DEST_D | COMP_D_MINUS_A);
		emitA	("ARG");
		emitC	(DEST_M | COMP_D);// ARG = SP - " << argc << " - 5\n"
		emitA	("R15");
		emitC	(DEST_A | COMP_M | JMP);
		argStubs.push_back(argc);
	}
	emitL	(returnAddress);
}

void VMWriter::writeReturn()
{
	emitA	("__return");
	emitC	(COMP_ZERO | JMP);
}
/*
 * BOOTSTRAP
 */
void VMWriter::writeBootstrap()
{
	std::string init("Sys.init");
	std::string ret("__return");

	emitA	(256);
	emitC	(DEST_D | COMP_A);
	emitA	("SP");
	emitC	(DEST_M | COMP_D);
	writeCall(StringTable::id(init), 0);
	emitL	(StringTable::id(ret));
	emitA	(5);
	emitC	(DEST_D | COMP_A);
	emitA	("LCL");
	emitC	(DEST_A | COMP_M_MINUS_D);
	emitC	(DEST_D | COMP_M);
	emitA	("R15");
	emitC	(DEST_M | COMP_D);// R15 = *(LCL-5)
	emitA	("SP");
	emitC	(DEST_A | DEST_M | COMP_M_MINUS_ONE);
	emitC	(DEST_D | COMP_M);// return value
	emitA	("ARG");
	emitC	(DEST_A | COMP_M);
	emitC	(DEST_M | COMP_D);// *ARG = pop()
	emitC	(DEST_D | COMP_A_PLUS_ONE);
	emitA	("SP");
	emitC	(DEST_M | COMP_D);// SP = ARG + 1
	emitA	("LCL");
	emitC	(DEST_D | COMP_M);
	emitA	("R14");
	emitC	(DEST_A | DEST_M | COMP_D_MINUS_ONE);// R14 = LCL - 1
	emitC	(DEST_D | COMP_M);
	emitA	("THAT");
	emitC	(DEST_M | COMP_D);// THAT = M[R14]
	emitA	("R14");
	emitC	(DEST_A | DEST_M | COMP_M_MINUS_ONE);
	emitC	(DEST_D | COMP_M);
	emitA	("THIS");
	emitC	(DEST_M | COMP_D);// THIS = M[R14--]
	emitA	("R14");
	emitC	(DEST_A | DEST_M | COMP_M_MINUS_ONE);
	emitC	(DEST_D | COMP_M);
	emitA	("ARG");
	emitC	(DEST_M | COMP_D);// ARG = M[R14--]
	emitA	("R14");
	emitC	(DEST_A | DEST_M | COMP_M_MINUS_ONE);
	emitC	(DEST_D | COMP_M);
	emitA	("LCL");
	emitC	(DEST_M | COMP_D);// LCL = M[R14--]
	emitA	("R15");
	emitC	(DEST_A | COMP_M | JMP);// goto R15
}
/*
 * Instruction emitting.
 * TODO: combine with assembler
 */
void VMWriter::emitA(const char *symbol) {
	out << "@" << symbol << std::endl;
}
void VMWriter::emitA(StringID &symbol) {
	out << "@" << symbol << std::endl;
}
void VMWriter::emitA(unsigned short constant) {
	if (constant & COMPUTE) {
		std::cout << "Warning! negative constant" << std::endl;
		out << "@" << abs((signed short)constant) << std::endl;
		emitC(DEST_A | COMP_MINUS_A);
	} else {
		out << "@" << constant << std::endl;
	}
}

void VMWriter::emitC(unsigned short instr) {
	if (instr & MASK_DEST) {
		if (instr & DEST_A)
			out << 'A';
		if (instr & DEST_M)
			out << 'M';
		if (instr & DEST_D)
			out << 'D';
		out << '=';
	}
	switch (instr & MASK_COMP) {
	case COMP_ZERO:
		out << "0";
		break;
	case COMP_ONE:
		out << "1";
		break;
	case COMP_MINUS_ONE:
		out << "-1";
		break;
	case COMP_D:
		out << "D";
		break;
	case COMP_A:
		out << "A";
		break;
	case COMP_NOT_D:
		out << "!D";
		break;
	case COMP_NOT_A:
		out << "!A";
		break;
	case COMP_MINUS_D:
		out << "-D";
		break;
	case COMP_MINUS_A:
		out << "-A";
		break;
	case COMP_D_PLUS_ONE:
		out << "D+1";
		break;
	case COMP_A_PLUS_ONE:
		out << "A+1";
		break;
	case COMP_D_MINUS_ONE:
		out << "D-1";
		break;
	case COMP_A_MINUS_ONE:
		out << "A-1";
		break;
	case COMP_D_PLUS_A:
		out << "D+A";
		break;
	case COMP_D_MINUS_A:
		out << "D-A";
		break;
	case COMP_A_MINUS_D:
		out << "A-D";
		break;
	case COMP_D_AND_A:
		out << "D&A";
		break;
	case COMP_D_OR_A:
		out << "D|A";
		break;
	case COMP_M:
		out << "M";
		break;
	case COMP_NOT_M:
		out << "!M";
		break;
	case COMP_MINUS_M:
		out << "-M";
		break;
	case COMP_M_PLUS_ONE:
		out << "M+1";
		break;
	case COMP_M_MINUS_ONE:
		out << "M-1";
		break;
	case COMP_D_PLUS_M:
		out << "D+M";
		break;
	case COMP_D_MINUS_M:
		out << "D-M";
		break;
	case COMP_M_MINUS_D:
		out << "M-D";
		break;
	case COMP_D_AND_M:
		out << "D&M";
		break;
	case COMP_D_OR_M:
		out << "D|M";
		break;
	default:
		throw std::runtime_error("C-instruction uses undocumented computation");
	}
	switch (instr & MASK_JUMP) {
	case JLT:
		out << ";JLT\n";
		break;
	case JGT:
		out << ";JGT\n";
		break;
	case JLE:
		out << ";JLE\n";
		break;
	case JGE:
		out << ";JGE\n";
		break;
	case JNE:
		out << ";JNE\n";
		break;
	case JEQ:
		out << ";JEQ\n";
		break;
	case JMP:
		out << ";JMP\n";
		break;
	default:
		out << "\n";
		break;
	}
}
void VMWriter::emitL(StringID &label)
{
	out << "(" << label << ")\n";
}

} // end namespace
