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
#include "JackVMWriter.h"
#include <stdexcept>
#include <sstream>

namespace hcc {
namespace jack {

VMWriter::VMWriter(const char *file)
	: output(file)
{
}

void VMWriter::doClass(const std::string name)
{
	staticVarsCount = 0;
	fieldVarsCount = 0;
	symbols.beginScope();

	className = name;
}

void VMWriter::doVariableDec(VariableStorage storage, VariableType &type, const std::string name)
{
	Symbol symbol;
	symbol.storage = storage;
	symbol.type = type;

	switch (storage) {
	case STATIC:
		symbol.index = staticVarsCount;
		++staticVarsCount;
		break;
	case FIELD:
		symbol.index = fieldVarsCount;
		++fieldVarsCount;
		break;
	case ARGUMENT:
		symbol.index = argumentVarsCount;
		++argumentVarsCount;
		break;
	case LOCAL:
		symbol.index = localVarsCount;
		++localVarsCount;
		break;
	}
	symbols.insert(name, symbol);
}

void VMWriter::doSubroutineStart(SubroutineKind kind, VariableType &returnType, const std::string name)
{
	localVarsCount = 0;
	argumentVarsCount = 0;
	symbols.beginScope();

	whileCounter = 0;
	ifCounter = 0;

	subroutineKind = kind;
	subroutineName = name;
}
void VMWriter::doSubroutineAfterVarDec()
{
	output << "function " << className << "." << subroutineName << " " << localVarsCount << '\n';
	switch (subroutineKind) {
	case CONSTRUCTOR:
		output << "push constant " << fieldVarsCount << '\n'
		       << "call Memory.alloc 1\n"
		       << "pop pointer 0\n";
		argumentOffset = 0;
		break;
	case METHOD:
		output << "push argument 0\n"
		       << "pop pointer 0\n";
		argumentOffset = 1;
		break;
	case FUNCTION:
		argumentOffset = 0;
		break;
	}
}
void VMWriter::doSubroutineEnd()
{
	symbols.endScope();
}

/*
 * flow
 */
void VMWriter::doIf()
{
	ifStack.push(ifCounter++);
	output << "if-goto IF_TRUE" << ifStack.top() << '\n'
	       << "goto IF_FALSE" << ifStack.top() << '\n'
	       << "label IF_TRUE" << ifStack.top() << '\n';
}

void VMWriter::doElse()
{
	output << "goto IF_END" << ifStack.top() << '\n'
	       << "label IF_FALSE" << ifStack.top() << '\n';
}

void VMWriter::doEndif(bool hasElse)
{
	if (hasElse) {
		output << "label IF_END" << ifStack.top() << '\n';
	} else {
		output << "label IF_FALSE" << ifStack.top() << '\n';
	}
	ifStack.pop();
}

void VMWriter::doWhileExp()
{
	whileStack.push(whileCounter++);
	output << "label WHILE_EXP" << whileStack.top() << '\n';
}

void VMWriter::doWhile()
{
	output << "not\n"
	       << "if-goto WHILE_END" << whileStack.top() << '\n';
}

void VMWriter::doEndwhile()
{
	output << "goto WHILE_EXP" << whileStack.top() << '\n'
	       << "label WHILE_END" << whileStack.top() << '\n';
	whileStack.pop();
}

void VMWriter::doReturn(bool nonVoid)
{
	if (!nonVoid) {
		/* Subroutine always returns. Push arbitrary value to stack */
		output << "push constant 0\n";
	}
	output << "return\n";
}
/*
 * do & call
 */
void VMWriter::doDoSimpleStart()
{
	output << "push pointer 0\n";
}
void VMWriter::doDoSimpleEnd(const std::string name)
{
	output << "call " << className << "." << name << " " << (expressionsCount+1) << '\n';
	/* Do not spoil the stack!
	 * Each subroutine has a return value. Had we not popped it, stack could grow
	 * indefinitely if "do statement" is inside the loop */
	output << "pop temp 0\n";
}

std::string symbolToSegmentIndex(Symbol symbol, int argumentOffset)
{
	std::stringstream result;

	switch (symbol.storage) {
	case STATIC:
		result << "static " << symbol.index;
		break;
	case FIELD:
		result << "this " << symbol.index;
		break;
	case ARGUMENT:
		result << "argument " << (symbol.index + argumentOffset);
		break;
	case LOCAL:
		result << "local " << symbol.index;
		break;
	}
	return result.str();
}

void VMWriter::doDoCompoundStart(const std::string name)
{
	if (symbols.contains(name)) {
		Symbol symbol = symbols.get(name);
		output << "push " << symbolToSegmentIndex(symbol, argumentOffset) << '\n';
		doCompoundStart = symbol.type.name;
		doMethod = true;
	} else {
		doCompoundStart = name;
		doMethod = false;
	}
}

void VMWriter::doDoCompoundEnd(const std::string name)
{
	if (doMethod)
		++expressionsCount;

	output << "call " << doCompoundStart << "." << name << " " << expressionsCount << '\n';
	/* Do not spoil the stack!
	 * Each subroutine has a return value. Had we not popped it, stack could grow
	 * indefinitely if "do statement" is inside the loop */
	output << "pop temp 0\n";
}

void VMWriter::doCallCompoundStart(const std::string name)
{
	if (symbols.contains(name)) {
		Symbol symbol = symbols.get(name);
		output << "push " << symbolToSegmentIndex(symbol, argumentOffset) << '\n';
		callCompoundStart = symbol.type.name;
		callMethod = true;
	} else {
		callCompoundStart = name;
		callMethod = false;
	}
}

void VMWriter::doCallCompoundEnd(const std::string name)
{
	if (callMethod)
		++expressionsCount;

	output << "call " << callCompoundStart << "." << name << " " << expressionsCount << '\n';
}

void VMWriter::doLetScalar(const std::string name)
{
	if (!symbols.contains(name))
		throw std::runtime_error("no such variable");

	output << "pop " << symbolToSegmentIndex(symbols.get(name), argumentOffset) << '\n';
}

void VMWriter::doLetVectorStart(const std::string name)
{
	doVariableScalar(name);
	output << "add\n";
}
void VMWriter::doLetVectorEnd()
{
	output << "pop temp 0\n"
	       << "pop pointer 1\n"
	       << "push temp 0\n"
	       << "pop that 0\n";
}
/*
 * expressions
 */
void VMWriter::doVariableVector(const std::string name)
{
	if (!symbols.contains(name))
		throw std::runtime_error("no such variable");

	output << "push " << symbolToSegmentIndex(symbols.get(name), argumentOffset) << '\n'
	       << "add\n"
	       << "pop pointer 1\n"
	       << "push that 0\n";
}

void VMWriter::doVariableScalar(const std::string name)
{
	if (!symbols.contains(name))
		throw std::runtime_error("no such variable");

	output << "push " << symbolToSegmentIndex(symbols.get(name), argumentOffset) << '\n';
}

void VMWriter::doBinary(char op)
{
	switch (op) {
	case '+':
		output << "add\n";
		break;
	case '-':
		output << "sub\n";
		break;
	case '*':
		output << "call Math.multiply 2\n";
		break;
	case '/':
		output << "call Math.divide 2\n";
		break;
	case '&':
		output << "and\n";
		break;
	case '|':
		output << "or\n";
		break;
	case '<':
		output << "lt\n";
		break;
	case '>':
		output << "gt\n";
		break;
	case '=':
		output << "eq\n";
		break;
	default:
		throw std::runtime_error("Unknown binary operation.");
	}
}

void VMWriter::doNeg()
{
	output << "neg\n";
}

void VMWriter::doNot()
{
	output << "not\n";
}

void VMWriter::doThis()
{
	output << "push pointer 0\n";
}

void VMWriter::doIntConstant(int i)
{
	output << "push constant " << i << '\n';
}

void VMWriter::doStringConstant(const std::string str)
{
	unsigned int len = str.length();
	output << "push constant " << len << '\n'
	       << "call String.new 1\n";
	for (unsigned int i = 0; i<len; ++i) {
		output << "push constant " << (unsigned int)str.at(i) << '\n'
		       << "call String.appendChar 2\n";
	}

}

} // end namespace jack
} // end namespace hcc