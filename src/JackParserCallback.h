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
#include "StringTable.h"

namespace hcc {
namespace jack {

/* variable's logical type */
struct VariableType {
	typedef enum {VOID, INT, CHAR, BOOLEAN, AGGREGATE} Kind;

	Kind kind;
	StringID name; // for aggregate kind
};
std::ostream& operator<<(std::ostream &stream, VariableType &vartype);

/* variable's storage mode */
typedef enum {STATIC, FIELD, ARGUMENT, LOCAL} VariableStorage;
std::ostream& operator<<(std::ostream &stream, VariableStorage &storage);

/* subroutine's kind */
typedef enum {CONSTRUCTOR, FUNCTION, METHOD} SubroutineKind;
std::ostream& operator<<(std::ostream &stream, SubroutineKind &kind);

/* called by Parser */
struct ParserCallback {
	virtual ~ParserCallback();

	virtual void setExpressionsCount(unsigned int count) = 0;

	virtual void doClass(StringID &name) = 0;
	virtual void doVariableDec(VariableStorage storage, VariableType &type, StringID &name) = 0;
	virtual void doSubroutineStart(SubroutineKind kind, VariableType &returnType, StringID &name) = 0;
	virtual void doSubroutineAfterVarDec() = 0;

	virtual void doIf() = 0;
	virtual void doElse() = 0;
	virtual void doEndif(bool hasElse) = 0;
	virtual void doWhileExp() = 0;
	virtual void doWhile() = 0;
	virtual void doEndwhile() = 0;
	virtual void doReturn(bool nonVoid) = 0;

	virtual void doDoSimpleStart() = 0;
	virtual void doDoSimpleEnd(StringID &name) = 0;
	virtual void doDoCompoundStart(StringID &name) = 0;
	virtual void doDoCompoundEnd(StringID &name) = 0;
	virtual void doCallCompoundStart(StringID &name) = 0;
	virtual void doCallCompoundEnd(StringID &name) = 0;
	virtual void doLetScalar(StringID &name) = 0;
	virtual void doLetVectorStart(StringID &name) = 0;
	virtual void doLetVectorEnd() = 0;

	virtual void doVariableVector(StringID &name) = 0;
	virtual void doVariableScalar(StringID &name) = 0;
	virtual void doBinary(char op) = 0;
	virtual void doNeg() = 0;
	virtual void doNot() = 0;
	virtual void doThis() = 0;
	virtual void doIntConstant(int i) = 0;
	virtual void doStringConstant(StringID &s) = 0;
};

} // end namespace jack
} // end namespace hcc
