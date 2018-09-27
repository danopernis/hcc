// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/jack/parser.h"

#include "hcc/jack/ast.h"
#include "hcc/jack/tokenizer.h"

namespace hcc {
namespace jack {

namespace {

struct parser {
    explicit parser(tokenizer& t)
        : t(t)
    {
    }
    tokenizer& t;
    token current_token;

    bool accept_token(const token_type& tt)
    {
        if (current_token.type == tt) {
            current_token = t.advance();
            return true;
        }
        return false;
    }

    void expect_token(const token_type& tt)
    {
        if (!accept_token(tt)) {
            throw parse_error("Expected " + to_string(tt) + ", got "
                              + to_string(current_token.type),
                              t.pos.line, t.pos.column);
        }
    }

    bool accept_identifier(std::string& result)
    {
        if (current_token.type == token_type::IDENTIFIER) {
            result = std::move(current_token.string_value);
            current_token = t.advance();
            return true;
        }
        return false;
    }

    /**
     * Return identifier or throw an exception if there is none.
     */
    std::string expect_identifier()
    {
        std::string result;
        if (!accept_identifier(result)) {
            throw parse_error("Expected identifier", t.pos.line, t.pos.column);
        }
        return result;
    }

    /**
     * @grammar class = 'class' identifier '{' classVarDec* subroutineDec* '}'
     */
    Class parse_class()
    {
        Class result;

        current_token = t.advance();
        expect_token(token_type::CLASS);
        result.name = expect_identifier();

        expect_token(token_type::BRACE_LEFT);
        while (true) {
            if (accept_token(token_type::STATIC)) {
                parse_variable_declaration(result.staticVariables);
            } else if (accept_token(token_type::FIELD)) {
                parse_variable_declaration(result.fieldVariables);
            } else {
                break;
            }
        }
        while (true) {
            if (accept_token(token_type::CONSTRUCTOR)) {
                result.subroutines.push_back(parse_subroutine(Subroutine::Kind::CONSTRUCTOR));
            } else if (accept_token(token_type::FUNCTION)) {
                result.subroutines.push_back(parse_subroutine(Subroutine::Kind::FUNCTION));
            } else if (accept_token(token_type::METHOD)) {
                result.subroutines.push_back(parse_subroutine(Subroutine::Kind::METHOD));
            } else {
                break;
            }
        }
        expect_token(token_type::BRACE_RIGHT);
        expect_token(token_type::EOF_);

        return result;
    }

    /**
     * @grammar subroutineDec = ('void' | type) identifier '(' parameterList ')' '{'
     * variableDeclaration* statements '}'
     */
    Subroutine parse_subroutine(Subroutine::Kind kind)
    {
        Subroutine subroutine;
        subroutine.kind = kind;

        if (accept_token(token_type::VOID)) {
            subroutine.returnType = nullptr;
        } else {
            subroutine.returnType = expect_type();
        }

        subroutine.name = expect_identifier();

        parse_parameter_list(subroutine.arguments);
        expect_token(token_type::BRACE_LEFT);
        while (accept_token(token_type::VAR)) {
            parse_variable_declaration(subroutine.variables);
        }
        subroutine.statements = parse_statements();
        expect_token(token_type::BRACE_RIGHT);

        return subroutine;
    }

    /**
     * @grammar variableDeclaration = type identifier (',' identifier)* ';'
     */
    void parse_variable_declaration(VariableDeclarationList& declarations)
    {
        auto type = expect_type();

        do {
            VariableDeclaration vd;
            vd.name = expect_identifier();
            vd.type = type->clone();
            declarations.push_back(std::move(vd));
        } while (accept_token(token_type::COMMA));
        expect_token(token_type::SEMICOLON);
    }

    /**
     * @grammar parameterList = '(' type varName (',' type varName)* ')'
     */
    void parse_parameter_list(VariableDeclarationList& declarations)
    {
        expect_token(token_type::PARENTHESIS_LEFT);
        if (accept_token(token_type::PARENTHESIS_RIGHT))
            return;

        do {
            VariableDeclaration vd;
            vd.type = expect_type();
            vd.name = expect_identifier();

            declarations.push_back(std::move(vd));
        } while (accept_token(token_type::COMMA));
        expect_token(token_type::PARENTHESIS_RIGHT);
    }

    /**
     * @grammar statements = (letStatement | ifStatement | whileStatement | doStatement |
     * returnStatement)*
     */
    StatementList parse_statements()
    {
        StatementList statements;

        while (true) {
            if (accept_token(token_type::LET)) {
                parse_let_statement(statements);
            } else if (accept_token(token_type::IF)) {
                parse_if_statement(statements);
            } else if (accept_token(token_type::WHILE)) {
                parse_while_statement(statements);
            } else if (accept_token(token_type::DO)) {
                parse_do_statement(statements);
            } else if (accept_token(token_type::RETURN)) {
                parse_return_statement(statements);
            } else {
                break;
            }
        }

        return statements;
    }

    /**
     * @grammar letStatement = identifier ('[' expression ']')? '=' expression ';'
     */
    void parse_let_statement(StatementList& statements)
    {
        std::string name = expect_identifier();
        if (accept_token(token_type::BRACKET_LEFT)) {
            auto subscript = expect_expression();
            expect_token(token_type::BRACKET_RIGHT);
            expect_token(token_type::EQUALS_SIGN);

            statements.push_back(
                std::make_unique<LetVector>(name, std::move(subscript), expect_expression()));
        } else {
            expect_token(token_type::EQUALS_SIGN);

            statements.push_back(std::make_unique<LetScalar>(name, expect_expression()));
        }
        expect_token(token_type::SEMICOLON);
    }

    /**
     * @grammar ifStatement = '(' expression ')' '{' statements '}' ( 'else' '{' statements '}' )?
     */
    void parse_if_statement(StatementList& statements)
    {
        expect_token(token_type::PARENTHESIS_LEFT);
        Expression condition = expect_expression();
        expect_token(token_type::PARENTHESIS_RIGHT);

        expect_token(token_type::BRACE_LEFT);
        StatementList positive_branch = parse_statements();
        StatementList negative_branch;
        expect_token(token_type::BRACE_RIGHT);
        if (accept_token(token_type::ELSE)) {
            expect_token(token_type::BRACE_LEFT);
            negative_branch = parse_statements();
            expect_token(token_type::BRACE_RIGHT);
        }

        statements.push_back(std::make_unique<IfStatement>(
            std::move(condition), std::move(positive_branch), std::move(negative_branch)));
    }

    /**
     * @grammar whileStatement = '(' expression ')' '{' statements '}'
     */
    void parse_while_statement(StatementList& statements)
    {
        expect_token(token_type::PARENTHESIS_LEFT);
        Expression condition = expect_expression();
        expect_token(token_type::PARENTHESIS_RIGHT);

        expect_token(token_type::BRACE_LEFT);
        StatementList body = parse_statements();
        expect_token(token_type::BRACE_RIGHT);

        statements.push_back(
            std::make_unique<WhileStatement>(std::move(condition), std::move(body)));
    }

    /**
     * @grammar doStatement = subroutineCall ';'
     */
    void parse_do_statement(StatementList& statements)
    {
        std::string name = expect_identifier();
        std::string base;

        if (accept_token(token_type::DOT)) {
            base = name;
            name = expect_identifier();
        }
        expect_token(token_type::PARENTHESIS_LEFT);
        statements.push_back(std::make_unique<DoStatement>(base, name, parse_expression_list()));
        expect_token(token_type::PARENTHESIS_RIGHT);
        expect_token(token_type::SEMICOLON);
    }

    /**
     * @grammar returnStatement = (expression)? ';'
     */
    void parse_return_statement(StatementList& statements)
    {
        statements.push_back(std::make_unique<ReturnStatement>(accept_expression()));
        expect_token(token_type::SEMICOLON);
    }

    /**
     * Return type or throw an exception if there is none.
     *
     * @grammar type = 'int' | 'char' | 'boolean' | identifier
     */
    VariableType expect_type()
    {
        std::string identifier;
        if (accept_identifier(identifier)) {
            return std::make_unique<UnresolvedType>(identifier);
        } else if (accept_token(token_type::CHAR)) {
            return std::make_unique<CharType>();
        } else if (accept_token(token_type::BOOLEAN)) {
            return std::make_unique<BooleanType>();
        } else if (accept_token(token_type::INT)) {
            return std::make_unique<IntType>();
        }
        throw parse_error("Expected type", t.pos.line, t.pos.column);
    }

    /**
     * Maybe return expression.
     *
     * @grammar expression = term (op term)*
     * @grammar op = '+' | '-' | '*' | '/' | '&' | '|' | '<' | '>' | '='
     */
    Expression accept_expression()
    {
        auto expression = accept_term();
        if (!expression) {
            return Expression();
        }

        while (true) {
            BinaryExpression::Type type;
            if (accept_token(token_type::PLUS_SIGN)) {
                type = BinaryExpression::Type::ADD;
            } else if (accept_token(token_type::MINUS_SIGN)) {
                type = BinaryExpression::Type::SUBSTRACT;
            } else if (accept_token(token_type::ASTERISK)) {
                type = BinaryExpression::Type::MULTIPLY;
            } else if (accept_token(token_type::SLASH)) {
                type = BinaryExpression::Type::DIVIDE;
            } else if (accept_token(token_type::AMPERSAND)) {
                type = BinaryExpression::Type::AND;
            } else if (accept_token(token_type::VERTICAL_BAR)) {
                type = BinaryExpression::Type::OR;
            } else if (accept_token(token_type::LESS_THAN_SIGN)) {
                type = BinaryExpression::Type::LESSER;
            } else if (accept_token(token_type::GREATER_THAN_SIGN)) {
                type = BinaryExpression::Type::GREATER;
            } else if (accept_token(token_type::EQUALS_SIGN)) {
                type = BinaryExpression::Type::EQUAL;
            } else {
                // did not found op => finished
                return expression;
            }
            // found op => expect term & next round
            expression
                = std::make_unique<BinaryExpression>(type, std::move(expression), expect_term());
        }
    }

    /**
     * Return expression or throw an exception if there is none.
     */
    Expression expect_expression()
    {
        if (auto result = accept_expression()) {
            return result;
        }
        throw parse_error("Expected expression", t.pos.line, t.pos.column);
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
    Expression accept_term()
    {
        if (accept_token(token_type::TRUE)) {
            return std::make_unique<UnaryExpression>(UnaryExpression::Type::NOT,
                                                     std::make_unique<IntegerConstant>(0));
        }
        if (accept_token(token_type::FALSE) || accept_token(token_type::NULL_)) {
            return std::make_unique<IntegerConstant>(0);
        }
        if (accept_token(token_type::THIS)) {
            return std::make_unique<ThisConstant>();
        }
        if (accept_token(token_type::MINUS_SIGN)) {
            return std::make_unique<UnaryExpression>(UnaryExpression::Type::MINUS,
                                                     expect_term());
        }
        if (accept_token(token_type::TILDE)) {
            return std::make_unique<UnaryExpression>(UnaryExpression::Type::NOT,
                                                     expect_term());
        }
        if (current_token.type == token_type::INT_CONST) {
            auto value = current_token.int_value;
            current_token = t.advance();
            return std::make_unique<IntegerConstant>(value);
        }
        if (current_token.type == token_type::STRING_CONST) {
            auto value = current_token.string_value;
            current_token = t.advance();
            return std::make_unique<StringConstant>(value);
        }
        std::string name;
        if (accept_identifier(name)) {

            if (accept_token(token_type::BRACKET_LEFT)) {
                auto subscript = expect_expression();
                expect_token(token_type::BRACKET_RIGHT);

                return std::make_unique<VectorVariable>(name, std::move(subscript));
            }
            if (accept_token(token_type::DOT)) {
                std::string base = name;
                name = expect_identifier();
                expect_token(token_type::PARENTHESIS_LEFT);
                auto arguments = parse_expression_list();
                expect_token(token_type::PARENTHESIS_RIGHT);

                return std::make_unique<SubroutineCall>(base, name, std::move(arguments));
            }
            if (accept_token(token_type::PARENTHESIS_LEFT)) {
                auto arguments = parse_expression_list();
                expect_token(token_type::PARENTHESIS_RIGHT);
                return std::make_unique<SubroutineCall>("", name, std::move(arguments));
            }
            // else it is scalar variable access
            return std::make_unique<ScalarVariable>(name);
        }
        if (accept_token(token_type::PARENTHESIS_LEFT)) {
            auto expression = expect_expression();
            expect_token(token_type::PARENTHESIS_RIGHT);
            return expression;
        }

        return Expression(); // nothing
    }

    /**
     * Return term or throw an exception if there is none.
     */
    Expression expect_term()
    {
        if (auto result = accept_term()) {
            return result;
        }
        throw parse_error("Expected term", t.pos.line, t.pos.column);
    }

    /**
     * Return (potentially empty) expressionList.
     *
     * @grammar expressionList = (expression (',' expression)* )?
     */
    ExpressionList parse_expression_list()
    {
        ExpressionList result;

        if (auto expression = accept_expression()) {
            result.push_back(std::move(expression));
            while (accept_token(token_type::COMMA)) {
                result.push_back(expect_expression());
            }
        }

        return result;
    }
};

} // namespace {

Class parse(tokenizer& t)
{
    parser p{t};
    return p.parse_class();
}

} // namespace jack {
} // namespace hcc {
