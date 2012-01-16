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
#include "JackTokenizer.h"
#include <iostream>
#include <cctype>

namespace hcc {
namespace jack {

Tokenizer::Tokenizer(const std::string& filename)
{
	input.open(filename.c_str());
	state = START;
	line = 1;
	column = 0;

	keywords["class"]       = K_CLASS;
	keywords["method"]      = K_METHOD;
	keywords["function"]    = K_FUNCTION;
	keywords["constructor"] = K_CONSTRUCTOR;
	keywords["int"]         = K_INT;
	keywords["boolean"]     = K_BOOLEAN;
	keywords["char"]        = K_CHAR;
	keywords["void"]        = K_VOID;
	keywords["var"]         = K_VAR;
	keywords["static"]      = K_STATIC;
	keywords["field"]       = K_FIELD;
	keywords["let"]         = K_LET;
	keywords["do"]          = K_DO;
	keywords["if"]          = K_IF;
	keywords["else"]        = K_ELSE;
	keywords["while"]       = K_WHILE;
	keywords["return"]      = K_RETURN;
	keywords["true"]        = K_TRUE;
	keywords["false"]       = K_FALSE;
	keywords["null"]        = K_NULL;
	keywords["this"]        = K_THIS;
}

Tokenizer::~Tokenizer()
{
}

bool Tokenizer::hasMoreTokens()
{
	return input.good();
}

void Tokenizer::advance()
{
	type = T_NONE;

	while (type == T_NONE) {
		char c = (char)input.get();
		if (c == '\n') {
			column = 0;
			++line;
		} else {
			++column;
		}

		switch (state) {
		case START:
			if (c == '/') {
				state = SLASH;
			} else if (c == '"') {
				stringConstantStream.str("");
				state = STRING;
			} else if (isdigit(c)) {
				intConstant = c - '0';
				state = NUMBER;
			} else if (isalpha(c)) {
				identifierStream.str("");
				identifierStream << c;
				state = IDENTIFIER;
			} else if (isspace(c)) {
				// do nothing
			} else {
				type = T_SYMBOL;
				symbol = c;
			}
			break;
		case SLASH:
			if (c == '/') {
				state = COMMENT_ONELINE;
			} else if (c == '*') {
				state = COMMENT_MULTILINE;
			} else {
				type = T_SYMBOL;
				symbol = '/';

				input.putback(c);
				state = START;
			}
			break;
		case COMMENT_ONELINE:
			if (c == '\n') {
				state = START;
			}
			break;
		case COMMENT_MULTILINE:
			if (c == '*') {
				state = COMMENT_MULTILINE_END;
			}
			break;
		case COMMENT_MULTILINE_END:
			if (c == '/') {
				state = START;
			} else {
				state = COMMENT_MULTILINE;
			}
			break;
		case STRING:
			if (c == '"') {
				type = T_STRING_CONST;
				std::string s = stringConstantStream.str();
				stringConstant = StringTable::id(s);

				state = START;
			} else {
				stringConstantStream << c;
			}
			break;
		case NUMBER:
			if (isdigit(c)) {
				intConstant = 10*intConstant + (c - '0');
			} else {
				type = T_INT_CONST;

				input.putback(c);
				state = START;
			}
			break;
		case IDENTIFIER:
			if (isalnum(c)) {
				identifierStream << c;
			} else {
				std::string s = identifierStream.str();
				identifier = StringTable::id(s);
				KeywordMap::iterator kw = keywords.find(s);
				if (kw == keywords.end()) {
					type = T_IDENTIFIER;
				} else {
					type = T_KEYWORD;
					keyword = kw->second;
				}
				input.putback(c);
				state = START;
			}
			break;
		}
	}
}

} // end namespace jack
} // end namespace hcc
