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
#include "StringTable.h"
#include <stdexcept>

namespace hcc {

std::map<std::string, StringID> StringTable::string2id;
std::map<StringID, std::string> StringTable::id2string;

StringID& StringTable::id(std::string s) {
	if (string2id.find(s) == string2id.end()) {
		StringID &rv = string2id[s];
		rv.id = string2id.size();
		id2string[rv] = s;
		return rv;
	} else {
		return string2id[s];
	}
}
std::string& StringTable::string(StringID &i) {
	if (id2string.find(i) == id2string.end())
		throw std::runtime_error("non-existing string requested");
	return id2string[i];
}
std::ostream& operator<<(std::ostream &out, StringID &id)
{
	return out << StringTable::string(id);
}

} // end namespace
