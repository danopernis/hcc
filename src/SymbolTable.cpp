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
#include "SymbolTable.h"
#include <list>
#include <map>
#include <cassert>

namespace hcc {

typedef std::map<std::string, Symbol> Scope;
typedef	std::list<Scope> ScopeList;

class SymbolTable::impl {
private:
	impl(impl const &);
	impl& operator=(impl const &other);
public:
	impl() {}
	ScopeList scopes;
};

SymbolTable::SymbolTable()
	: pimpl(new SymbolTable::impl())
{
}
void SymbolTable::beginScope()
{
	Scope s;
	pimpl->scopes.push_front(s);
}

void SymbolTable::endScope()
{
	pimpl->scopes.pop_front();
}

void SymbolTable::insert(const std::string key, const Symbol &value)
{
	pimpl->scopes.front().insert(std::make_pair(key,value));
}

bool SymbolTable::contains(const std::string key) const
{
	for (ScopeList::iterator i = pimpl->scopes.begin(); i != pimpl->scopes.end(); ++i) {
		if (i->find(key) != i->end()) {
			return true;
		}
	}
	return false;
}

const Symbol& SymbolTable::get(const std::string key) const
{
	Scope::iterator result;
	for (ScopeList::iterator i = pimpl->scopes.begin(); i != pimpl->scopes.end(); ++i) {
		result = i->find(key);
		if (result != i->end()) {
			return result->second;
		}
	}
	assert(false);
}

} // ned namespace hcc
