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

namespace hcc {
namespace jack {

ParserCallback::~ParserCallback()
{
}

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

} // end namespace jack
} // end namespace hcc
