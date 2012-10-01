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
#include <string>
#include <list>
#include <iostream>
#include "instruction.h"

namespace hcc {

typedef enum {LOCAL, ARGUMENT, THIS, THAT, POINTER, TEMP, STATIC} Segment;
typedef enum {NEG, NOT, ADDC, SUBC, BUSC, ANDC, ORC, DOUBLE} UnaryOperation;
typedef enum {ADD, SUB, BUS, AND, OR} BinaryOperation;

struct CompareOperation {
	bool lt, eq, gt;

	virtual ~CompareOperation() {}
	void set(bool lt, bool eq, bool gt) {
		this->lt = lt;
		this->eq = eq;
		this->gt = gt;
	}
	CompareOperation& negate() {
		lt ^= true;
		eq ^= true;
		gt ^= true;

		return (*this);
	}
	CompareOperation& swap() {
		lt ^= true;
		gt ^= true;

		return (*this);
	}
	unsigned short jump() {
	if (lt) {
		if (eq)
			return gt ? instruction::JMP : instruction::JLE;
		else
			return gt ? instruction::JNE : instruction::JLT;
	} else {
		if (eq)
			return gt ? instruction::JGE : instruction::JEQ;
		else
			return gt ? instruction::JGT : 0;
	}
}
};

class VMCommand {
public:
	typedef enum {NOP, CONSTANT, UNARY, BINARY, COMPARE, PUSH, POP_DIRECT, POP_INDIRECT, COPY, LABEL, GOTO, IF,
	UNARY_COMPARE, UNARY_COMPARE_IF, COMPARE_IF, FUNCTION, RETURN, CALL, IN, FIN, POP_INDIRECT_PUSH} Type;

	std::string arg1;
	Type type;
	UnaryOperation unary;
	BinaryOperation binary;
	CompareOperation compare;
	Segment segment1, segment2;
	int int1, int2;

	bool in, fin;
};

typedef std::list<VMCommand> VMCommandList;

std::ostream& operator<<(std::ostream &out, VMCommand &c);

} // end namespace
