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

Parser::Parser(Tokenizer &tokenizer, ParserCallback &callback)
	: tokenizer(tokenizer)
	, callback(callback)
{
}

/*
 * shortcuts
 */
void Parser::next()
{
	if (!tokenizer.hasMoreTokens())
		throw ParseError("Unexpected end of file", tokenizer);

	lastKeyword        = tokenizer.getKeyword();
	lastSymbol         = tokenizer.getSymbol();
	lastStringConstant = tokenizer.getStringConstant();
	lastIdentifier     = tokenizer.getIdentifier();
	lastIntConstant    = tokenizer.getIntConstant();

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
	if (acceptKeyword(k))
		return;
	
	std::stringstream message;
	message << "Expected keyword " << k;
	throw ParseError(message.str(), tokenizer);
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
	if (acceptIdentifier())
		return;

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
	if (acceptSymbol(s))
		return;

	std::stringstream message;
	message << "Expected symbol " << s;
	throw ParseError(message.str(), tokenizer);
}

/*
 * grammar
 */
void Parser::parseClass()
{
	expectKeyword(Tokenizer::K_CLASS);
	expectIdentifier();
	callback.doClass(lastIdentifier);

	expectSymbol('{');
	for (;;) { // class variables
		if (acceptKeyword(Tokenizer::K_STATIC)) {
			parseVariable(STATIC);
		} else if (acceptKeyword(Tokenizer::K_FIELD)) {
			parseVariable(FIELD);
		} else {
			break;
		}
	}
	for (;;) { // subroutines
		if (acceptKeyword(Tokenizer::K_CONSTRUCTOR)) {
			parseSubroutine(CONSTRUCTOR);
		} else if (acceptKeyword(Tokenizer::K_FUNCTION)) {
			parseSubroutine(FUNCTION);
		} else if (acceptKeyword(Tokenizer::K_METHOD)) {
			parseSubroutine(METHOD);
		} else {
			break;
		}
	}
	expectSymbol('}');
}
void Parser::parseVariable(VariableStorage storage)
{
	expectType();
	do {
		expectIdentifier();
		callback.doVariableDec(storage, lastType, lastIdentifier);
	} while (acceptSymbol(','));
	expectSymbol(';');
}
void Parser::parseSubroutine(SubroutineKind kind)
{
	if (acceptKeyword(Tokenizer::K_VOID)) {
		lastType.kind = VariableType::VOID;
	} else {
		expectType();
	}
	expectIdentifier();
	callback.doSubroutineStart(kind, lastType, lastIdentifier);

	parseArgumentList();
	expectSymbol('{');

	while (acceptKeyword(Tokenizer::K_VAR)) {
		parseVariable(LOCAL);
	}

	callback.doSubroutineAfterVarDec();
	parseStatements();
	expectSymbol('}');
	callback.doSubroutineEnd();
}

void Parser::parseArgumentList()
{
	expectSymbol('(');
	if (acceptSymbol(')'))
		return;

	do {
		expectType();
		expectIdentifier();
		callback.doVariableDec(ARGUMENT, lastType, lastIdentifier);
	} while (acceptSymbol(','));
	expectSymbol(')');
}

void Parser::parseStatements()
{
	for (;;) {
		if (acceptKeyword(Tokenizer::K_LET)) {
			expectIdentifier();
			std::string name = lastIdentifier;
			if (acceptSymbol('[')) {
				expectExpression();
				expectSymbol(']');
				callback.doLetVectorStart(name);
				expectSymbol('=');
				expectExpression();
				expectSymbol(';');
				callback.doLetVectorEnd();
			} else {
				expectSymbol('=');
				expectExpression();
				expectSymbol(';');
				callback.doLetScalar(name);
			}
		} else if (acceptKeyword(Tokenizer::K_IF)) {
			expectSymbol('(');
			expectExpression();
			expectSymbol(')');

			callback.doIf();
			expectSymbol('{');
			parseStatements();
			expectSymbol('}');
			if (acceptKeyword(Tokenizer::K_ELSE)) {
				callback.doElse();
				expectSymbol('{');
				parseStatements();
				expectSymbol('}');
				callback.doEndif(true);
			} else {
				callback.doEndif(false);
			}
		} else if (acceptKeyword(Tokenizer::K_WHILE)) {
			callback.doWhileExp();

			expectSymbol('(');
			expectExpression();
			expectSymbol(')');
			callback.doWhile();

			expectSymbol('{');
			parseStatements();
			expectSymbol('}');
			callback.doEndwhile();
		} else if (acceptKeyword(Tokenizer::K_DO)) {
			expectIdentifier();
			std::string name = lastIdentifier;

			if (acceptSymbol('(')) {
				callback.doDoSimpleStart();
				parseExpressionList();
				expectSymbol(')');
				expectSymbol(';');
				callback.doDoSimpleEnd(name);
			} else if (acceptSymbol('.')) {
				callback.doDoCompoundStart(name);
				expectIdentifier();
				name = lastIdentifier;
				expectSymbol('(');
				parseExpressionList();
				expectSymbol(')');
				expectSymbol(';');
				callback.doDoCompoundEnd(name);
			} else {
				throw ParseError("Expected subroutine call", tokenizer);
			}
		} else if (acceptKeyword(Tokenizer::K_RETURN)) {
			if (acceptExpression()) {
				callback.doReturn(true);
			} else {
				callback.doReturn(false);
			}
			expectSymbol(';');
		} else {
			break;
		}
	}
}

bool Parser::acceptType()
{
	if (acceptKeyword(Tokenizer::K_INT)) {
		lastType.kind = VariableType::INT;
		return true;
	}
	if (acceptKeyword(Tokenizer::K_CHAR)) {
		lastType.kind = VariableType::CHAR;
		return true;
	}
	if (acceptKeyword(Tokenizer::K_BOOLEAN)) {
		lastType.kind = VariableType::BOOLEAN;
		return true;
	}
	if (acceptIdentifier()) {
		lastType.kind = VariableType::AGGREGATE;
		lastType.name = lastIdentifier;
		return true;
	}
	return false;
}
void Parser::expectType()
{
	if (acceptType())
		return;

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
			char op = lastSymbol;
			expectTerm();
			callback.doBinary(op);
		}
		return true;
	}
	return false;
}
void Parser::expectExpression()
{
	if (acceptExpression())
		return;
	
	throw ParseError("Expected expression", tokenizer);
}

bool Parser::acceptTerm()
{
	if (acceptKeyword(Tokenizer::K_TRUE)) {
		callback.doIntConstant(0);
		callback.doNot();
		return true; // keyword constant
	}
	if (acceptKeyword(Tokenizer::K_FALSE) ||
	    acceptKeyword(Tokenizer::K_NULL)) {
		callback.doIntConstant(0);
		return true; // keyword constant
	}
	if (acceptKeyword(Tokenizer::K_THIS)) {
		callback.doThis();
		return true; // keyword constant
	}
	if (acceptIdentifier()) {
		std::string name = lastIdentifier;

		if (acceptSymbol('[')) {
			expectExpression();
			expectSymbol(']');
			callback.doVariableVector(name);
			return true; // array access
		}
		if (acceptSymbol('.')) {
			callback.doCallCompoundStart(name);
			expectIdentifier();
			name = lastIdentifier;
			expectSymbol('(');
			parseExpressionList();
			expectSymbol(')');
			callback.doCallCompoundEnd(name);
			return true; // subroutine call
		}
		if (acceptSymbol('(')) {
			parseExpressionList();
			expectSymbol(')');
			return true; // method call
		}
		// else it is scalar variable access
		callback.doVariableScalar(name);
		return true;
	}
	if (acceptSymbol('(')) {
		expectExpression();
		expectSymbol(')');
		return true; // expression
	}
	if (acceptSymbol('-')) {
		expectTerm();
		callback.doNeg();
		return true; // unary minus
	}
	if (acceptSymbol('~')) {
		expectTerm();
		callback.doNot();
		return true; // unary not
	}
	if (tokenizer.getTokenType() == Tokenizer::T_INT_CONST) {
		next();
		callback.doIntConstant(lastIntConstant);
		return true; // integer constant
	}
	if (tokenizer.getTokenType() == Tokenizer::T_STRING_CONST) {
		next();
		callback.doStringConstant(lastStringConstant);
		return true; // string constant
	}

	return false; // not accepted
}
void Parser::expectTerm()
{
	if (acceptTerm())
		return;

	throw ParseError("Expected term", tokenizer);
}

void Parser::parseExpressionList()
{
	unsigned int expressionsCount = 0;
	if (acceptExpression()) {
		++expressionsCount;
		while (acceptSymbol(',')) {
			expectExpression();
			++expressionsCount;
		}
	}
	callback.setExpressionsCount(expressionsCount);
}

/*
 * interface
 */
void Parser::parse()
{
	try {
		next();
		parseClass();
	} catch (ParseError &e) {
		std::cerr << "Parse error: " << e.what() << " at " << e.line << ":" << e.column << '\n';
	}
}

} // end namespace jack
} // end namespace hcc
