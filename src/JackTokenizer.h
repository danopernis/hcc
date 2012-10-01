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
#include <string>
#include <map>
#include <fstream>
#include <sstream>

namespace hcc {
namespace jack {

class Tokenizer {
public:
	typedef enum {T_NONE, T_KEYWORD, T_SYMBOL, T_IDENTIFIER, T_INT_CONST, T_STRING_CONST} TokenType;
	typedef enum {K_CLASS, K_METHOD, K_FUNCTION, K_CONSTRUCTOR, K_INT, K_BOOLEAN,
	              K_CHAR, K_VOID, K_VAR, K_STATIC, K_FIELD, K_LET, K_DO, K_IF,
		      K_ELSE, K_WHILE, K_RETURN, K_TRUE, K_FALSE, K_NULL, K_THIS} Keyword;
	typedef enum {START, SLASH, COMMENT_ONELINE, COMMENT_MULTILINE, COMMENT_MULTILINE_END,
	              STRING, NUMBER, IDENTIFIER} State;

private:
	std::ifstream input;
	State state;
	typedef std::map<std::string, Keyword> KeywordMap;
	KeywordMap keywords;

	TokenType type;
	Keyword keyword;
	char symbol;
	std::stringstream identifierStream, stringConstantStream;
	std::string identifier, stringConstant;
	int intConstant;
	unsigned int line, column;

public:

	Tokenizer(const std::string& filename);
	virtual ~Tokenizer();

	bool hasMoreTokens();
	void advance();

	TokenType getTokenType() const {
		return type;
	}

	Keyword getKeyword() const {
		return keyword;
	}

	char getSymbol() const {
		return symbol;
	}

	const std::string getIdentifier() const {
		return identifier;
	}

	const std::string getStringConstant() const {
		return stringConstant;
	}

	int getIntConstant() const {
		return intConstant;
	}

	unsigned int getLine() const {
		return line;
	}

	unsigned int getColumn() const {
		return column;
	}
};

} // end namespace jack
} // end namespace hcc
