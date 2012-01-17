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
#include <fstream>
#include <stack>

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
class ParserCallback {
	// flow control
	unsigned int whileCounter, ifCounter, expressionsCount, argumentOffset;
	std::stack<unsigned int> whileStack, ifStack;

	// variable scoping
	unsigned int staticVarsCount, fieldVarsCount, argumentVarsCount, localVarsCount;
	std::map<StringID, unsigned int> staticVarNames, fieldVarNames, argumentVarNames, localVarNames;
	std::map<StringID, VariableType> staticVarTypes, fieldVarTypes, argumentVarTypes, localVarTypes;

	bool doMethod, callMethod;
	StringID className, subroutineName, doCompoundStart, callCompoundStart;
	SubroutineKind subroutineKind;
	std::ofstream output;
public:
	ParserCallback(const char *file);
	void setExpressionsCount(unsigned int count) {
		expressionsCount = count;
	}

	void doClass(StringID &name);
	void doVariableDec(VariableStorage storage, VariableType &type, StringID &name);
	void doSubroutineStart(SubroutineKind kind, VariableType &returnType, StringID &name);
	void doSubroutineAfterVarDec();

	void doIf();
	void doElse();
	void doEndif(bool hasElse);
	void doWhileExp();
	void doWhile();
	void doEndwhile();
	void doReturn(bool nonVoid);

	void doDoSimpleStart();
	void doDoSimpleEnd(StringID &name);
	void doDoCompoundStart(StringID &name);
	void doDoCompoundEnd(StringID &name);
	void doCallCompoundStart(StringID &name);
	void doCallCompoundEnd(StringID &name);
	void doLetScalar(StringID &name);
	void doLetVectorStart(StringID &name);
	void doLetVectorEnd();

	void doVariableVector(StringID &name);
	void doVariableScalar(StringID &name);
	void doBinary(char op);
	void doNeg();
	void doNot();
	void doThis();
	void doIntConstant(int i);
	void doStringConstant(StringID &s);
};

} // end namespace jack
} // end namespace hcc
