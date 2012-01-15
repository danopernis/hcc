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

JackParser::JackParser(JackTokenizer& tokenizer)
	: tokenizer(tokenizer)
{
}

/*
 * shortcuts
 */
void JackParser::next()
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
bool JackParser::acceptKeyword(JackTokenizer::Keyword k)
{
	if (tokenizer.getTokenType() == JackTokenizer::T_KEYWORD &&
	    tokenizer.getKeyword() == k) {
		next();
		return true;
	}
	return false;
}
void JackParser::expectKeyword(JackTokenizer::Keyword k)
{
	if (!acceptKeyword(k))
		throw ParseError("Unexpected keyword", tokenizer);
}
bool JackParser::acceptIdentifier()
{
	if (tokenizer.getTokenType() == JackTokenizer::T_IDENTIFIER) {
		next();
		return true;
	}
	return false;
}
void JackParser::expectIdentifier()
{
	if (!acceptIdentifier())
		throw ParseError("Expected identifier", tokenizer);
}
bool JackParser::acceptSymbol(char s)
{
	if (tokenizer.getTokenType() == JackTokenizer::T_SYMBOL &&
	    tokenizer.getSymbol() == s) {
		next();
		return true;
	}
	return false;
}
void JackParser::expectSymbol(char s)
{
	if (!acceptSymbol(s))
		throw ParseError("Expected symbol", tokenizer);
}
/*
 * grammar
 */
void JackParser::parseClass()
{
	next();
	expectKeyword(JackTokenizer::K_CLASS);
	expectIdentifier();
	c.name = lastIdentifier;

	expectSymbol('{');
nextClassVarDec:
	if (acceptKeyword(JackTokenizer::K_STATIC)) {
		parseVariable(Variable::STATIC, c.variables);
		goto nextClassVarDec;
	}
	if (acceptKeyword(JackTokenizer::K_FIELD)) {
		parseVariable(Variable::FIELD, c.variables);
		goto nextClassVarDec;
	}
nextSubroutineDec:
	if (acceptKeyword(JackTokenizer::K_CONSTRUCTOR)) {
		parseSubroutine(Subroutine::CONSTRUCTOR, c.subroutines);
		goto nextSubroutineDec;
	}
	if (acceptKeyword(JackTokenizer::K_FUNCTION)) {
		parseSubroutine(Subroutine::FUNCTION, c.subroutines);
		goto nextSubroutineDec;
	}
	if (acceptKeyword(JackTokenizer::K_METHOD)) {
		parseSubroutine(Subroutine::METHOD, c.subroutines);
		goto nextSubroutineDec;
	}
	expectSymbol('}');
}
void JackParser::parseVariable(Variable::Kind k, VariableList &l) {
	Variable v;
	v.kind = k;

	expectType();
	v.type = lastType;

nextVar:
	expectIdentifier();
	v.name = lastIdentifier;
	l.push_back(v);
		
	if (acceptSymbol(',')) {
		goto nextVar;
	}
	expectSymbol(';');
}
void JackParser::parseSubroutine(Subroutine::Kind k, SubroutineList &l)
{
	Subroutine s;

	if (acceptKeyword(JackTokenizer::K_VOID)) {
		;
	} else {
		expectType();
	}
	expectIdentifier();
	s.name = lastIdentifier;

	expectSymbol('(');
	parseParameterList();
	expectSymbol(')');
	expectSymbol('{');

nextVarDec:
	if (acceptKeyword(JackTokenizer::K_VAR)) {
		parseVariable(Variable::LOCAL, s.localVars);
		goto nextVarDec;
	}
	parseStatements();
	expectSymbol('}');

	l.push_back(s);
}

void JackParser::parseParameterList()
{
	if (acceptType()) {
		expectIdentifier();
		while (acceptSymbol(',')) {
			expectType();
			expectIdentifier();
		}
	}
}

void JackParser::parseStatements()
{
nextStatement:
	if (acceptKeyword(JackTokenizer::K_LET)) {
		/*
		 * append to current block
		 */
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
	if (acceptKeyword(JackTokenizer::K_IF)) {
		expectSymbol('(');
		expectExpression();
		expectSymbol(')');
		expectSymbol('{');
		parseStatements();
		expectSymbol('}');
		if (acceptKeyword(JackTokenizer::K_ELSE)) {
			expectSymbol('{');
			parseStatements();
			expectSymbol('}');
		}
		goto nextStatement;
	}
	if (acceptKeyword(JackTokenizer::K_WHILE)) {
		expectSymbol('(');
		expectExpression();
		expectSymbol(')');
		expectSymbol('{');
		parseStatements();
		expectSymbol('}');
		goto nextStatement;
	}
	if (acceptKeyword(JackTokenizer::K_DO)) {
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
	if (acceptKeyword(JackTokenizer::K_RETURN)) {
		if (acceptExpression()) {
		
		}
		expectSymbol(';');
		goto nextStatement;
	}
}

bool JackParser::acceptType()
{
	if (acceptKeyword(JackTokenizer::K_INT)) {
		lastType.type = VariableType::INT;
		return true;
	}
	if (acceptKeyword(JackTokenizer::K_CHAR)) {
		lastType.type = VariableType::CHAR;
		return true;
	}
	if (acceptKeyword(JackTokenizer::K_BOOLEAN)) {
		lastType.type = VariableType::BOOLEAN;
		return true;
	}
	if (acceptIdentifier()) {
		lastType.type = VariableType::CUSTOM;
		lastType.name = lastIdentifier;
		return true;
	}
	return false;
}
void JackParser::expectType()
{
	if (!acceptType())
		throw ParseError("Expected type", tokenizer);
}

bool JackParser::acceptExpression()
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
void JackParser::expectExpression()
{
	if (!acceptExpression())
		throw ParseError("Expected expression", tokenizer);
}

bool JackParser::acceptTerm()
{
	if (acceptKeyword(JackTokenizer::K_TRUE) ||
	    acceptKeyword(JackTokenizer::K_FALSE) ||
	    acceptKeyword(JackTokenizer::K_NULL) ||
	    acceptKeyword(JackTokenizer::K_THIS)) {
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
		return true; // variable access
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
	if (tokenizer.getTokenType() == JackTokenizer::T_INT_CONST) {
		next();
		return true; // integer constant
	}
	if (tokenizer.getTokenType() == JackTokenizer::T_STRING_CONST) {
		next();
		return true; // string constant
	}

	return false; // not accepted
}
void JackParser::expectTerm()
{
	if (!acceptTerm())
		throw ParseError("Expected term", tokenizer);
}

void JackParser::parseExpressionList()
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
void JackParser::parse()
{
	try {
		parseClass();
	} catch (ParseError &e) {
		std::cerr << "Parse error: " << e.what() << " at " << e.line << ":" << e.column << std::endl;
	}
}

} // end namespace
