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
#include "JackParserCallback.h"
#include <fstream>
#include <stack>

namespace hcc {
namespace jack {

class VMWriter : public ParserCallback {
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
	VMWriter(const char *file);
	virtual void setExpressionsCount(unsigned int count) {
		expressionsCount = count;
	}

	virtual void doClass(StringID &name);
	virtual void doVariableDec(VariableStorage storage, VariableType &type, StringID &name);
	virtual void doSubroutineStart(SubroutineKind kind, VariableType &returnType, StringID &name);
	virtual void doSubroutineAfterVarDec();

	virtual void doIf();
	virtual void doElse();
	virtual void doEndif(bool hasElse);
	virtual void doWhileExp();
	virtual void doWhile();
	virtual void doEndwhile();
	virtual void doReturn(bool nonVoid);

	virtual void doDoSimpleStart();
	virtual void doDoSimpleEnd(StringID &name);
	virtual void doDoCompoundStart(StringID &name);
	virtual void doDoCompoundEnd(StringID &name);
	virtual void doCallCompoundStart(StringID &name);
	virtual void doCallCompoundEnd(StringID &name);
	virtual void doLetScalar(StringID &name);
	virtual void doLetVectorStart(StringID &name);
	virtual void doLetVectorEnd();

	virtual void doVariableVector(StringID &name);
	virtual void doVariableScalar(StringID &name);
	virtual void doBinary(char op);
	virtual void doNeg();
	virtual void doNot();
	virtual void doThis();
	virtual void doIntConstant(int i);
	virtual void doStringConstant(StringID &s);
};

} // end namespace jack
} // end namespace hcc
