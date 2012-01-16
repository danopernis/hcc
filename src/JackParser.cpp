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
#include "JackParser.h"
#include <iostream>

namespace hcc {
namespace jack {

Parser::Parser(Tokenizer& tokenizer)
	: tokenizer(tokenizer)
{
}

/*
 * shortcuts
 */
void Parser::next()
{
	if (!tokenizer.hasMoreTokens())
		throw ParseError("Unexpected end of file.", tokenizer);

	lastKeyword = tokenizer.getKeyword();
	lastSymbol = tokenizer.getSymbol();
	lastStringConstant = tokenizer.getStringConstant();
	lastIdentifier = tokenizer.getIdentifier();
	lastIntConstant = tokenizer.getIntConstant();
	tokenizer.advance();
}
bool Parser::acceptKeyword(Tokenizer::Keyword k)
{
	if (tokenizer.getTokenType() == Tokenizer::T_KEYWORD &&
	    tokenizer.getKeyword() == k) {
		next();
		return true;
	}
	return false;
}
void Parser::expectKeyword(Tokenizer::Keyword k)
{
	if (!acceptKeyword(k))
		throw ParseError("Expected keyword", tokenizer);
}
bool Parser::acceptIdentifier()
{
	if (tokenizer.getTokenType() == Tokenizer::T_IDENTIFIER) {
		next();
		return true;
	}
	return false;
}
void Parser::expectIdentifier()
{
	if (!acceptIdentifier())
		throw ParseError("Expected identifier", tokenizer);
}
bool Parser::acceptSymbol(char s)
{
	if (tokenizer.getTokenType() == Tokenizer::T_SYMBOL &&
	    tokenizer.getSymbol() == s) {
		next();
		return true;
	}
	return false;
}
void Parser::expectSymbol(char s)
{
	if (!acceptSymbol(s))
		throw ParseError("Expected symbol", tokenizer);
}
/*
 * grammar
 */
void Parser::parseClass()
{
	next();
	expectKeyword(Tokenizer::K_CLASS);
	expectIdentifier();

	expectSymbol('{');
nextClassVarDec:
	if (acceptKeyword(Tokenizer::K_STATIC)) {
		parseVariable();
		goto nextClassVarDec;
	}
	if (acceptKeyword(Tokenizer::K_FIELD)) {
		parseVariable();
		goto nextClassVarDec;
	}
nextSubroutineDec:
	if (acceptKeyword(Tokenizer::K_CONSTRUCTOR)) {
		parseSubroutine();
		goto nextSubroutineDec;
	}
	if (acceptKeyword(Tokenizer::K_FUNCTION)) {
		parseSubroutine();
		goto nextSubroutineDec;
	}
	if (acceptKeyword(Tokenizer::K_METHOD)) {
		parseSubroutine();
		goto nextSubroutineDec;
	}
	expectSymbol('}');
}
void Parser::parseVariable()
{
	expectType();
nextVar:
	expectIdentifier();
		
	if (acceptSymbol(',')) {
		goto nextVar;
	}
	expectSymbol(';');
}
void Parser::parseSubroutine()
{
	if (acceptKeyword(Tokenizer::K_VOID)) {
		;
	} else {
		expectType();
	}
	expectIdentifier();

	expectSymbol('(');
	parseArgumentList();
	expectSymbol(')');
	expectSymbol('{');

nextVarDec:
	if (acceptKeyword(Tokenizer::K_VAR)) {
		parseVariable();
		goto nextVarDec;
	}
	parseStatements();
	expectSymbol('}');
}

void Parser::parseArgumentList()
{
	if (acceptType()) {
		// first argument
		expectIdentifier();

		while (acceptSymbol(',')) {
			// each other argument
			expectType();
			expectIdentifier();
		}
	}
}

void Parser::parseStatements()
{
nextStatement:
	if (acceptKeyword(Tokenizer::K_LET)) {
		expectIdentifier();
		if (acceptSymbol('[')) {
			expectExpression();
			expectSymbol(']');
		}
		expectSymbol('=');
		expectExpression();
		expectSymbol(';');
		goto nextStatement;
	}
	if (acceptKeyword(Tokenizer::K_IF)) {
		expectSymbol('(');
		expectExpression();
		expectSymbol(')');
		expectSymbol('{');
		parseStatements();
		expectSymbol('}');
		if (acceptKeyword(Tokenizer::K_ELSE)) {
			expectSymbol('{');
			parseStatements();
			expectSymbol('}');
		}
		goto nextStatement;
	}
	if (acceptKeyword(Tokenizer::K_WHILE)) {
		expectSymbol('(');
		expectExpression();
		expectSymbol(')');
		expectSymbol('{');
		parseStatements();
		expectSymbol('}');
		goto nextStatement;
	}
	if (acceptKeyword(Tokenizer::K_DO)) {
		expectIdentifier();
		if (acceptSymbol('(')) {
			parseExpressionList();
			expectSymbol(')');
			expectSymbol(';');
			goto nextStatement;
		}
		if (acceptSymbol('.')) {
			expectIdentifier();
			expectSymbol('(');
			parseExpressionList();
			expectSymbol(')');
			expectSymbol(';');
			goto nextStatement;
		}
		throw ParseError("Expected subroutine call", tokenizer);
	}
	if (acceptKeyword(Tokenizer::K_RETURN)) {
		if (acceptExpression()) {
		}
		expectSymbol(';');
		goto nextStatement;
	}
}

bool Parser::acceptType()
{
	if (acceptKeyword(Tokenizer::K_INT)) {
		return true;
	}
	if (acceptKeyword(Tokenizer::K_CHAR)) {
		return true;
	}
	if (acceptKeyword(Tokenizer::K_BOOLEAN)) {
		return true;
	}
	if (acceptIdentifier()) {
		return true;
	}
	return false;
}
void Parser::expectType()
{
	if (!acceptType())
		throw ParseError("Expected type", tokenizer);
}

bool Parser::acceptExpression()
{
	if (acceptTerm()) {
		while (acceptSymbol('+') ||
		       acceptSymbol('-') ||
		       acceptSymbol('*') ||
		       acceptSymbol('/') ||
		       acceptSymbol('&') ||
		       acceptSymbol('|') ||
		       acceptSymbol('<') ||
		       acceptSymbol('>') ||
		       acceptSymbol('=')) {
			expectTerm();
		}
		return true;
	}
	return false;
}
void Parser::expectExpression()
{
	if (!acceptExpression())
		throw ParseError("Expected expression", tokenizer);
}

bool Parser::acceptTerm()
{
	if (acceptKeyword(Tokenizer::K_TRUE) ||
	    acceptKeyword(Tokenizer::K_FALSE) ||
	    acceptKeyword(Tokenizer::K_NULL) ||
	    acceptKeyword(Tokenizer::K_THIS)) {
		return true; // keyword constant
	}
	if (acceptIdentifier()) {
		if (acceptSymbol('[')) {
			expectExpression();
			expectSymbol(']');
			return true; // array access
		}
		if (acceptSymbol('.')) {
			expectIdentifier();
			expectSymbol('(');
			parseExpressionList();
			expectSymbol(')');
			return true; // subroutine call
		}
		if (acceptSymbol('(')) {
			parseExpressionList();
			expectSymbol(')');
			return true; // method call
		}
		// else it is variable access
		return true;
	}
	if (acceptSymbol('(')) {
		expectExpression();
		expectSymbol(')');
		return true; // expression
	}
	if (acceptSymbol('-') ||
	    acceptSymbol('~')) {
		expectTerm();
		return true; // unary expression
	}
	if (tokenizer.getTokenType() == Tokenizer::T_INT_CONST) {
		next();
		return true; // integer constant
	}
	if (tokenizer.getTokenType() == Tokenizer::T_STRING_CONST) {
		next();
		return true; // string constant
	}

	return false; // not accepted
}
void Parser::expectTerm()
{
	if (!acceptTerm())
		throw ParseError("Expected term", tokenizer);
}

void Parser::parseExpressionList()
{
	if (acceptExpression()) {
		while (acceptSymbol(',')) {
			expectExpression();
		}
	}
}

/*
 * interface
 */
void Parser::parse()
{
	try {
		parseClass();
	} catch (ParseError &e) {
		std::cerr << "Parse error: " << e.what() << " at " << e.line << ":" << e.column << std::endl;
	}
}

} // end namespace jack
} // end namespace hcc
