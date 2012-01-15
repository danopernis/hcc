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
#include <list>
#include "StringTable.h"

namespace hcc {

struct VariableType {
	typedef enum {INT, CHAR, BOOLEAN, CUSTOM} Type;

	Type type;
	StringID name;
};

struct Variable {
	typedef enum {STATIC, FIELD, PARAMETER, LOCAL} Kind;

	Kind kind;
	VariableType type;
	StringID name;
};
typedef std::list<Variable> VariableList;

struct Subroutine {
	typedef enum {CONSTRUCTOR, FUNCTION, METHOD} Kind;

	Kind kind;
	StringID name;
	VariableList parameters;
	VariableList localVars;
};
typedef std::list<Subroutine> SubroutineList;

struct Class {
	StringID name;
	VariableList variables;
	SubroutineList subroutines;
};

} // end namespace
