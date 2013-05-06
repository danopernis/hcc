/*
 * Copyright (c) 2012-2013 Dano Pernis
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
#include "make_unique.h"
#include <sstream>
#include <fstream>
#include <boost/optional.hpp>


namespace hcc {
namespace jack {


/** pimpl idiom */
struct Parser::Impl {
    /** The source of tokens for parser. */
    Tokenizer* tokenizer;


    /**
     * If current token is the given keyword, advance() and return true.
     * Otherwise return false.
     */
    bool acceptToken(TokenType tt)
    {
        if (tokenizer->getTokenType() == tt) {
            tokenizer->advance();
            return true;
        }
        return false;
    }


    /**
     * Throw an exception if current token is not the given keyword.
     */
    void expectToken(TokenType tt)
    {
        if (acceptToken(tt))
            return;

        std::stringstream message;
        message << "Expected " << tt << ", got " << tokenizer->getTokenType();
        throw ParseError(message.str(), *tokenizer);
    }


    /**
     * Maybe return identifier.
     */
    using MaybeIdentifier = boost::optional<std::string>;
    MaybeIdentifier acceptIdentifier()
    {
        MaybeIdentifier result;
        if (tokenizer->getTokenType() == TokenType::IDENTIFIER) {
            result = tokenizer->getIdentifier();
            tokenizer->advance();
        }
        return result;
    }


    /**
     * Return identifier or throw an exception if there is none.
     */
    std::string expectIdentifier()
    {
        if (auto result = acceptIdentifier())
            return *result;
        else
            throw ParseError("Expected identifier", *tokenizer);
    }


    /**
     * @grammar class = 'class' identifier '{' classVarDec* subroutineDec* '}'
     */
    ast::Class parseClass()
    {
        ast::Class result;

        expectToken(TokenType::CLASS);
        result.name = expectIdentifier();

        expectToken(TokenType::BRACE_LEFT);
        while (true) {
            if (acceptToken(TokenType::STATIC)) {
                parseVariableDeclaration(result.staticVariables);
            } else if (acceptToken(TokenType::FIELD)) {
                parseVariableDeclaration(result.fieldVariables);
            } else {
                break;
            }
        }
        while (true) {
            if (acceptToken(TokenType::CONSTRUCTOR)) {
                result.subroutines.push_back(parseSubroutine(ast::Subroutine::Kind::CONSTRUCTOR));
            } else if (acceptToken(TokenType::FUNCTION)) {
                result.subroutines.push_back(parseSubroutine(ast::Subroutine::Kind::FUNCTION));
            } else if (acceptToken(TokenType::METHOD)) {
                result.subroutines.push_back(parseSubroutine(ast::Subroutine::Kind::METHOD));
            } else {
                break;
            }
        }
        expectToken(TokenType::BRACE_RIGHT);
        expectToken(TokenType::EOF_);

        return result;
    }


    /**
     * @grammar subroutineDec = ('void' | type) identifier '(' parameterList ')' '{' variableDeclaration* statements '}'
     */
    ast::Subroutine parseSubroutine(ast::Subroutine::Kind kind)
    {
        ast::Subroutine subroutine;
        subroutine.kind = kind;

        if (acceptToken(TokenType::VOID)) {
            subroutine.returnType = nullptr;
        } else {
            subroutine.returnType = expectType();
        }

        subroutine.name = expectIdentifier();

        parseParameterList(subroutine.arguments);
        expectToken(TokenType::BRACE_LEFT);
        while (acceptToken(TokenType::VAR)) {
            parseVariableDeclaration(subroutine.variables);
        }
        subroutine.statements = parseStatements();
        expectToken(TokenType::BRACE_RIGHT);

        return subroutine;
    }


    /**
     * @grammar variableDeclaration = type identifier (',' identifier)* ';'
     */
    void parseVariableDeclaration(ast::VariableDeclarationList& declarations)
    {
        auto type = expectType();

        do {
            ast::VariableDeclaration vd;
            vd.name = expectIdentifier();
            vd.type = type->clone();
            declarations.push_back(std::move(vd));
        } while (acceptToken(TokenType::COMMA));
        expectToken(TokenType::SEMICOLON);
    }


    /**
     * @grammar parameterList = '(' type varName (',' type varName)* ')'
     */
    void parseParameterList(ast::VariableDeclarationList& declarations)
    {
        expectToken(TokenType::PARENTHESIS_LEFT);
        if (acceptToken(TokenType::PARENTHESIS_RIGHT))
            return;

        do {
            ast::VariableDeclaration vd;
            vd.type = expectType();
            vd.name = expectIdentifier();

            declarations.push_back(std::move(vd));
        } while (acceptToken(TokenType::COMMA));
        expectToken(TokenType::PARENTHESIS_RIGHT);
    }


    /**
     * @grammar statements = (letStatement | ifStatement | whileStatement | doStatement | returnStatement)*
     */
    ast::StatementList parseStatements()
    {
        ast::StatementList statements;

        while (true) {
            if (acceptToken(TokenType::LET)) {
                parseLetStatement(statements);
            } else if (acceptToken(TokenType::IF)) {
                parseIfStatement(statements);
            } else if (acceptToken(TokenType::WHILE)) {
                parseWhileStatement(statements);
            } else if (acceptToken(TokenType::DO)) {
                parseDoStatement(statements);
            } else if (acceptToken(TokenType::RETURN)) {
                parseReturnStatement(statements);
            } else {
                break;
            }
        }

        return statements;
    }


    /**
     * @grammar letStatement = identifier ('[' expression ']')? '=' expression ';'
     */
    void parseLetStatement(ast::StatementList& statements)
    {
        std::string name = expectIdentifier();
        if (acceptToken(TokenType::BRACKET_LEFT)) {
            auto subscript = expectExpression();
            expectToken(TokenType::BRACKET_RIGHT);
            expectToken(TokenType::EQUALS_SIGN);

            statements.push_back(make_unique<ast::LetVector>(
                name,
                std::move(subscript),
                expectExpression()
            ));
        } else {
            expectToken(TokenType::EQUALS_SIGN);

            statements.push_back(make_unique<ast::LetScalar>(
                name,
                expectExpression()
            ));
        }
        expectToken(TokenType::SEMICOLON);
    }


    /**
     * @grammar ifStatement = '(' expression ')' '{' statements '}' ( 'else' '{' statements '}' )?
     */
    void parseIfStatement(ast::StatementList& statements)
    {
        expectToken(TokenType::PARENTHESIS_LEFT);
        ast::Expression condition = expectExpression();
        expectToken(TokenType::PARENTHESIS_RIGHT);

        expectToken(TokenType::BRACE_LEFT);
        ast::StatementList positiveBranch = parseStatements();
        ast::StatementList negativeBranch;
        expectToken(TokenType::BRACE_RIGHT);
        if (acceptToken(TokenType::ELSE)) {
            expectToken(TokenType::BRACE_LEFT);
            negativeBranch = parseStatements();
            expectToken(TokenType::BRACE_RIGHT);
        }

        statements.push_back(make_unique<ast::IfStatement>(
            std::move(condition),
            std::move(positiveBranch),
            std::move(negativeBranch)
        ));
    }


    /**
     * @grammar whileStatement = '(' expression ')' '{' statements '}'
     */
    void parseWhileStatement(ast::StatementList& statements)
    {
        expectToken(TokenType::PARENTHESIS_LEFT);
        ast::Expression condition = expectExpression();
        expectToken(TokenType::PARENTHESIS_RIGHT);

        expectToken(TokenType::BRACE_LEFT);
        ast::StatementList body = parseStatements();
        expectToken(TokenType::BRACE_RIGHT);

        statements.push_back(make_unique<ast::WhileStatement>(
            std::move(condition),
            std::move(body)
        ));
    }


    /**
     * @grammar doStatement = subroutineCall ';'
     */
    void parseDoStatement(ast::StatementList& statements)
    {
        std::string name = expectIdentifier();
        std::string base;

        if (acceptToken(TokenType::DOT)) {
            base = name;
            name = expectIdentifier();
        }
        expectToken(TokenType::PARENTHESIS_LEFT);
        statements.push_back(make_unique<ast::DoStatement>(
            base,
            name,
            parseExpressionList()
        ));
        expectToken(TokenType::PARENTHESIS_RIGHT);
        expectToken(TokenType::SEMICOLON);
    }


    /**
     * @grammar returnStatement = (expression)? ';'
     */
    void parseReturnStatement(ast::StatementList& statements)
    {
        statements.push_back(make_unique<ast::ReturnStatement>(
            acceptExpression()
        ));
        expectToken(TokenType::SEMICOLON);
    }


    /**
     * Return type or throw an exception if there is none.
     *
     * @grammar type = 'int' | 'char' | 'boolean' | identifier
     */
    ast::VariableType expectType()
    {
        if (auto identifier = acceptIdentifier()) {
            return make_unique<ast::UnresolvedType>(*identifier);
        } else if (acceptToken(TokenType::CHAR)) {
            return make_unique<ast::CharType>();
        } else if (acceptToken(TokenType::BOOLEAN)) {
            return make_unique<ast::BooleanType>();
        } else if (acceptToken(TokenType::INT)) {
            return make_unique<ast::IntType>();
        } else {
            throw ParseError("Expected type", *tokenizer);
        }
    }


    /**
     * Maybe return expression.
     *
     * @grammar expression = term (op term)*
     * @grammar op = '+' | '-' | '*' | '/' | '&' | '|' | '<' | '>' | '='
     */
    ast::Expression acceptExpression()
    {
        auto expression = acceptTerm();
        if (!expression)
            return ast::Expression();

        while (true) {
            ast::BinaryExpression::Type type;
            if (acceptToken(TokenType::PLUS_SIGN)) {
                type = ast::BinaryExpression::Type::ADD;
            } else if (acceptToken(TokenType::MINUS_SIGN)) {
                type = ast::BinaryExpression::Type::SUBSTRACT;
            } else if (acceptToken(TokenType::ASTERISK)) {
                type = ast::BinaryExpression::Type::MULTIPLY;
            } else if (acceptToken(TokenType::SLASH)) {
                type = ast::BinaryExpression::Type::DIVIDE;
            } else if (acceptToken(TokenType::AMPERSAND)) {
                type = ast::BinaryExpression::Type::AND;
            } else if (acceptToken(TokenType::VERTICAL_BAR)) {
                type = ast::BinaryExpression::Type::OR;
            } else if (acceptToken(TokenType::LESS_THAN_SIGN)) {
                type = ast::BinaryExpression::Type::LESSER;
            } else if (acceptToken(TokenType::GREATER_THAN_SIGN)) {
                type = ast::BinaryExpression::Type::GREATER;
            } else if (acceptToken(TokenType::EQUALS_SIGN)) {
                type = ast::BinaryExpression::Type::EQUAL;
            } else {
                // did not found op => finished
                return expression;
            }
            // found op => expect term & next round
            expression = make_unique<ast::BinaryExpression>(
                type,
                std::move(expression),
                expectTerm()
            );
        }
    }


    /**
     * Return expression or throw an exception if there is none.
     */
    ast::Expression expectExpression()
    {
        if (auto result = acceptExpression())
            return result;
        else
            throw ParseError("Expected expression", *tokenizer);
    }


    /**
     * Maybe return term.
     *
     * @grammar term = integerConstant | stringConstant | keywordConstant
     *               | identifier ('[' expression ']')? | subroutineCall
     *               | '(' expression ')' | unaryOp term
     * @grammar subroutineCall = identifier ('.' identifier)? '(' expressionList ')'
     * @grammar unaryOp = '-' | '~'
     * @grammar keywordConstant = 'true' | 'false' | 'null' | 'this'
     */
    ast::Expression acceptTerm()
    {
        if (acceptToken(TokenType::TRUE)) {
            return make_unique<ast::UnaryExpression>(
                ast::UnaryExpression::Type::NOT,
                make_unique<ast::IntegerConstant>(0)
            );
        }
        if (acceptToken(TokenType::FALSE) || acceptToken(TokenType::NULL_)) {
            return make_unique<ast::IntegerConstant>(0);
        }
        if (acceptToken(TokenType::THIS)) {
            return make_unique<ast::ThisConstant>();
        }
        if (acceptToken(TokenType::MINUS_SIGN)) {
            return make_unique<ast::UnaryExpression>(
                ast::UnaryExpression::Type::MINUS,
                expectTerm()
            );
        }
        if (acceptToken(TokenType::TILDE)) {
            return make_unique<ast::UnaryExpression>(
                ast::UnaryExpression::Type::NOT,
                expectTerm()
            );
        }
        if (tokenizer->getTokenType() == TokenType::INT_CONST) {
            auto value = tokenizer->getIntConstant();
            tokenizer->advance();
            return make_unique<ast::IntegerConstant>(value);
        }
        if (tokenizer->getTokenType() == TokenType::STRING_CONST) {
            auto value = tokenizer->getStringConstant();
            tokenizer->advance();
            return make_unique<ast::StringConstant>(value);
        }
        if (auto identifier = acceptIdentifier()) {
            std::string name = *identifier;

            if (acceptToken(TokenType::BRACKET_LEFT)) {
                auto subscript = expectExpression();
                expectToken(TokenType::BRACKET_RIGHT);

                return make_unique<ast::VectorVariable>(
                    name,
                    std::move(subscript)
                );
            }
            if (acceptToken(TokenType::DOT)) {
                std::string base = name;
                name = expectIdentifier();
                expectToken(TokenType::PARENTHESIS_LEFT);
                auto arguments = parseExpressionList();
                expectToken(TokenType::PARENTHESIS_RIGHT);

                return make_unique<ast::SubroutineCall>(
                    base,
                    name,
                    std::move(arguments)
                );
            }
            if (acceptToken(TokenType::PARENTHESIS_LEFT)) {
                parseExpressionList();
                expectToken(TokenType::PARENTHESIS_RIGHT);
                throw std::runtime_error("Unimplemented");
            }
            // else it is scalar variable access
            return make_unique<ast::ScalarVariable>(name);
        }
        if (acceptToken(TokenType::PARENTHESIS_LEFT)) {
            auto expression = expectExpression();
            expectToken(TokenType::PARENTHESIS_RIGHT);
            return expression;
        }

        return ast::Expression(); // nothing
    }


    /**
     * Return term or throw an exception if there is none.
     */
    ast::Expression expectTerm()
    {
        if (auto result = acceptTerm())
            return result;
        else
            throw ParseError("Expected term", *tokenizer);
    }


    /**
     * Return (potentially empty) expressionList.
     *
     * @grammar expressionList = (expression (',' expression)* )?
     */
    ast::ExpressionList parseExpressionList()
    {
        ast::ExpressionList result;

        if (auto expression = acceptExpression()) {
            result.push_back(std::move(expression));
            while (acceptToken(TokenType::COMMA)) {
                result.push_back(expectExpression());
            }
        }

        return result;
    }
}; // end struct Parser::Impl


Parser::Parser()
    : pimpl(new Impl())
{}


// destructor must be defined in implementation file for pimpl + unique to work
Parser::~Parser() = default;


// implementation
ast::Class Parser::parse(const std::string& filename)
{
    std::ifstream input(filename.c_str());
    Tokenizer tokenizer(input);
    pimpl->tokenizer = &tokenizer;
    pimpl->tokenizer->advance();
    return pimpl->parseClass();
}


} // end namespace jack
} // end namespace hcc
