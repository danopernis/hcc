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
#include "JackParserCallback.h"
#include <stdexcept>

namespace hcc {
namespace jack {

/*
 * pretty printing
 */
std::ostream& operator<<(std::ostream &stream, VariableType &vartype)
{
	switch (vartype.kind) {
	case VariableType::VOID:
		stream << "VariableType(VOID)";
		break;
	case VariableType::INT:
		stream << "VariableType(INT)";
		break;
	case VariableType::CHAR:
		stream << "VariableType(CHAR)";
		break;
	case VariableType::BOOLEAN:
		stream << "VariableType(BOOLEAN)";
		break;
	case VariableType::AGGREGATE:
		stream << "VariableType(AGGREGATE, " << vartype.name << ")";
		break;
	}
	return stream;
}

std::ostream& operator<<(std::ostream &stream, VariableStorage storage)
{
	switch (storage) {
	case STATIC:
		stream << "VariableStorage(STATIC)";
		break;
	case FIELD:
		stream << "VariableStorage(FIELD)";
		break;
	case ARGUMENT:
		stream << "VariableStorage(ARGUMENT)";
		break;
	case LOCAL:
		stream << "VariableStorage(LOCAL)";
		break;
	}
	return stream;
}

std::ostream& operator<<(std::ostream &stream, SubroutineKind kind)
{
	switch (kind) {
	case CONSTRUCTOR:
		stream << "SubroutineType(CONSTRUCTOR)";
		break;
	case FUNCTION:
		stream << "SubroutineType(FUNCTION)";
		break;
	case METHOD:
		stream << "SubroutineType(METHOD)";
		break;
	}
	return stream;
}
/*
 * callbacks
 */
ParserCallback::ParserCallback(const char *file)
	: output(file)
{
}

void ParserCallback::doClass(StringID &name)
{
	staticVarsCount = 0;
	fieldVarsCount = 0;
	className = name;
}

void ParserCallback::doVariableDec(VariableStorage storage, VariableType &type, StringID &name)
{
	switch (storage) {
	case STATIC:
		staticVarNames[name] = staticVarsCount;
		staticVarTypes[name] = type;
		++staticVarsCount;
		break;
	case FIELD:
		fieldVarNames[name] = fieldVarsCount;
		fieldVarTypes[name] = type;
		++fieldVarsCount;
		break;
	case ARGUMENT:
		argumentVarNames[name] = argumentVarsCount;
		argumentVarTypes[name] = type;
		++argumentVarsCount;
		break;
	case LOCAL:
		localVarNames[name] = localVarsCount;
		localVarTypes[name] = type;
		++localVarsCount;
		break;
	}
}

void ParserCallback::doSubroutineStart(SubroutineKind kind, VariableType &returnType, StringID &name)
{
	localVarsCount = 0;
	localVarNames.clear();

	argumentVarsCount = 0;
	argumentVarNames.clear();

	whileCounter = 0;
	ifCounter = 0;

	subroutineKind = kind;
	subroutineName = name;
}
void ParserCallback::doSubroutineAfterVarDec()
{
	output << "function " << className << "." << subroutineName << " " << localVarsCount << std::endl;
	switch (subroutineKind) {
	case CONSTRUCTOR:
		output << "push constant " << fieldVarsCount << std::endl;
		output << "call Memory.alloc 1" << std::endl;
		output << "pop pointer 0" << std::endl;
		argumentOffset = 0;
		break;
	case METHOD:
		output << "push argument 0" << std::endl;
		output << "pop pointer 0" << std::endl;
		argumentOffset = 1;
		break;
	case FUNCTION:
		argumentOffset = 0;
		break;
	}
}
/*
 * flow
 */
void ParserCallback::doIf()
{
	ifStack.push(ifCounter++);
	output << "if-goto IF_TRUE" << ifStack.top() << std::endl;
	output << "goto IF_FALSE" << ifStack.top() << std::endl;
	output << "label IF_TRUE" << ifStack.top() << std::endl;
}

void ParserCallback::doElse()
{
	output << "goto IF_END" << ifStack.top() << std::endl;
	output << "label IF_FALSE" << ifStack.top() << std::endl;
}

void ParserCallback::doEndif(bool hasElse)
{
	if (hasElse) {
		output << "label IF_END" << ifStack.top() << std::endl;
	} else {
		output << "label IF_FALSE" << ifStack.top() << std::endl;
	}
	ifStack.pop();
}

void ParserCallback::doWhileExp()
{
	whileStack.push(whileCounter++);
	output << "label WHILE_EXP" << whileStack.top() << std::endl;
}

void ParserCallback::doWhile()
{
	output << "not" << std::endl;
	output << "if-goto WHILE_END" << whileStack.top() << std::endl;
}

void ParserCallback::doEndwhile()
{
	output << "goto WHILE_EXP" << whileStack.top() << std::endl;
	output << "label WHILE_END" << whileStack.top() << std::endl;
	whileStack.pop();
}

void ParserCallback::doReturn(bool nonVoid)
{
	if (!nonVoid) {
		/* Subroutine always returns. Push arbitrary value to stack */
		output << "push constant 0" << std::endl;
	}
	output << "return" << std::endl;
}
/*
 * do & call
 */
void ParserCallback::doDoSimpleStart()
{
	output << "push pointer 0" << std::endl;
}
void ParserCallback::doDoSimpleEnd(StringID &name)
{
	output << "call " << className << "." << name << " " << (expressionsCount+1) << std::endl;
	/* Do not spoil the stack!
	 * Each subroutine has a return value. Had we not popped it, stack could grow
	 * indefinitely if "do statement" is inside the loop */
	output << "pop temp 0" << std::endl;
}

void ParserCallback::doDoCompoundStart(StringID &name)
{
	if (staticVarNames.count(name) == 1) {
		output << "push static " << staticVarNames[name] << std::endl;
		doCompoundStart = staticVarTypes[name].name;
		doMethod = true;
		return;
	}
	if (fieldVarNames.count(name) == 1) {
		output << "push this " << fieldVarNames[name] << std::endl;
		doCompoundStart = fieldVarTypes[name].name;
		doMethod = true;
		return;
	}
	if (argumentVarNames.count(name) == 1) {
		output << "push argument " << (argumentVarNames[name]+argumentOffset) << std::endl;
		doCompoundStart = argumentVarTypes[name].name;
		doMethod = true;
		return;
	}
	if (localVarNames.count(name) == 1) {
		output << "push local " << localVarNames[name] << std::endl;
		doCompoundStart = localVarTypes[name].name;
		doMethod = true;
		return;
	}

	doCompoundStart = name;
	doMethod = false;
}

void ParserCallback::doDoCompoundEnd(StringID &name)
{
	if (doMethod)
		++expressionsCount;

	output << "call " << doCompoundStart << "." << name << " " << expressionsCount << std::endl;
	/* Do not spoil the stack!
	 * Each subroutine has a return value. Had we not popped it, stack could grow
	 * indefinitely if "do statement" is inside the loop */
	output << "pop temp 0" << std::endl;
}

void ParserCallback::doCallCompoundStart(StringID &name)
{
	if (staticVarNames.count(name) == 1) {
		output << "push static " << staticVarNames[name] << std::endl;
		callCompoundStart = staticVarTypes[name].name;
		callMethod = true;
		return;
	}
	if (fieldVarNames.count(name) == 1) {
		output << "push this " << fieldVarNames[name] << std::endl;
		callCompoundStart = fieldVarTypes[name].name;
		callMethod = true;
		return;
	}
	if (argumentVarNames.count(name) == 1) {
		output << "push argument " << (argumentVarNames[name]+argumentOffset) << std::endl;
		callCompoundStart = argumentVarTypes[name].name;
		callMethod = true;
		return;
	}
	if (localVarNames.count(name) == 1) {
		output << "push local " << localVarNames[name] << std::endl;
		callCompoundStart = localVarTypes[name].name;
		callMethod = true;
		return;
	}

	callCompoundStart = name;
	callMethod = false;
}

void ParserCallback::doCallCompoundEnd(StringID &name)
{
	if (callMethod)
		++expressionsCount;
	output << "call " << callCompoundStart << "." << name << " " << expressionsCount << std::endl;
}

void ParserCallback::doLetScalar(StringID &name)
{
	if (staticVarNames.count(name) == 1) {
		output << "pop static " << staticVarNames[name] << std::endl;
		return;
	}
	if (fieldVarNames.count(name) == 1) {
		output << "pop this " << fieldVarNames[name] << std::endl;
		return;
	}
	if (argumentVarNames.count(name) == 1) {
		output << "pop argument " << (argumentVarNames[name]+argumentOffset) << std::endl;
		return;
	}
	if (localVarNames.count(name) == 1) {
		output << "pop local " << localVarNames[name] << std::endl;
		return;
	}
	throw std::runtime_error("no such variable");
}

void ParserCallback::doLetVectorStart(StringID &name)
{
	doVariableScalar(name);
	output << "add" << std::endl;
}
void ParserCallback::doLetVectorEnd()
{
	output << "pop temp 0" << std::endl;
	output << "pop pointer 1" << std::endl;
	output << "push temp 0" << std::endl;
	output << "pop that 0" << std::endl;
}
/*
 * expressions
 */
void ParserCallback::doVariableVector(StringID &name)
{
	if (staticVarNames.count(name) == 1) {
		output << "push static " << staticVarNames[name] << std::endl;
		goto fin;
	}
	if (fieldVarNames.count(name) == 1) {
		output << "push this " << fieldVarNames[name] << std::endl;
		goto fin;
	}
	if (argumentVarNames.count(name) == 1) {
		output << "push argument " << (argumentVarNames[name]+argumentOffset) << std::endl;
		goto fin;
	}
	if (localVarNames.count(name) == 1) {
		output << "push local " << localVarNames[name] << std::endl;
		goto fin;
	}
	throw std::runtime_error("no such variable");
fin:
	output << "add" << std::endl;
	output << "pop pointer 1" << std::endl;
	output << "push that 0" << std::endl;
}

void ParserCallback::doVariableScalar(StringID &name)
{
	if (staticVarNames.count(name) == 1) {
		output << "push static " << staticVarNames[name] << std::endl;
		return;
	}
	if (fieldVarNames.count(name) == 1) {
		output << "push this " << fieldVarNames[name] << std::endl;
		return;
	}
	if (argumentVarNames.count(name) == 1) {
		output << "push argument " << (argumentVarNames[name]+argumentOffset) << std::endl;
		return;
	}
	if (localVarNames.count(name) == 1) {
		output << "push local " << localVarNames[name] << std::endl;
		return;
	}
	throw std::runtime_error("no such variable");
}

void ParserCallback::doBinary(char op)
{
	switch (op) {
	case '+':
		output << "add" << std::endl;
		break;
	case '-':
		output << "sub" << std::endl;
		break;
	case '*':
		output << "call Math.multiply 2" << std::endl;
		break;
	case '/':
		output << "call Math.divide 2" << std::endl;
		break;
	case '&':
		output << "and" << std::endl;
		break;
	case '|':
		output << "or" << std::endl;
		break;
	case '<':
		output << "lt" << std::endl;
		break;
	case '>':
		output << "gt" << std::endl;
		break;
	case '=':
		output << "eq" << std::endl;
		break;
	default:
		throw std::runtime_error("Unknown binary operation.");
	}
}

void ParserCallback::doNeg()
{
	output << "neg" << std::endl;
}

void ParserCallback::doNot()
{
	output << "not" << std::endl;
}

void ParserCallback::doThis()
{
	output << "push pointer 0" << std::endl;
}

void ParserCallback::doIntConstant(int i)
{
	output << "push constant " << i << std::endl;
}

void ParserCallback::doStringConstant(StringID &s)
{
	std::string str = StringTable::string(s);
	unsigned int len = str.length();
	output << "push constant " << len << std::endl;
	output << "call String.new 1" << std::endl;
	for (unsigned int i = 0; i<len; ++i) {
		output << "push constant " << (unsigned int)str.at(i) << std::endl;
		output << "call String.appendChar 2" << std::endl;
	}

}

} // end namespace jack
} // end namespace hcc
