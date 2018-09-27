// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#pragma once
#include <memory>
#include <string>
#include <vector>

namespace hcc {
namespace jack {
namespace ast {

/** Forward declarations for visitor. */
struct IntType;
struct CharType;
struct BooleanType;
struct UnresolvedType;

/** Visitor pattern for variable types. */
struct VariableTypeVisitor {
    virtual ~VariableTypeVisitor() = default;
    virtual void visit(IntType*) = 0;
    virtual void visit(CharType*) = 0;
    virtual void visit(BooleanType*) = 0;
    virtual void visit(UnresolvedType*) = 0;
};

/** Abstract base of all variable types. */
struct VariableTypeBase {
    virtual ~VariableTypeBase() = default;
    virtual void accept(VariableTypeVisitor*) = 0;
    virtual std::unique_ptr<VariableTypeBase> clone() = 0;
};
using VariableType = std::unique_ptr<VariableTypeBase>;

struct IntType : VariableTypeBase {
    virtual void accept(VariableTypeVisitor* visitor) { visitor->visit(this); }
    virtual VariableType clone() { return std::make_unique<IntType>(); }
};

struct CharType : VariableTypeBase {
    virtual void accept(VariableTypeVisitor* visitor) { visitor->visit(this); }
    virtual VariableType clone() { return std::make_unique<CharType>(); }
};

struct BooleanType : VariableTypeBase {
    virtual void accept(VariableTypeVisitor* visitor) { visitor->visit(this); }
    virtual VariableType clone() { return std::make_unique<BooleanType>(); }
};

struct UnresolvedType : VariableTypeBase {
    UnresolvedType(const std::string& name)
        : name(name)
    {
    }

    virtual void accept(VariableTypeVisitor* visitor) { visitor->visit(this); }
    virtual VariableType clone() { return std::make_unique<UnresolvedType>(name); }

    std::string name;
};

struct VariableDeclaration {
    VariableType type;
    std::string name;
};
using VariableDeclarationList = std::vector<VariableDeclaration>;

/******************************************************************************
 *
 * Expressions
 *
 *****************************************************************************/

/** Forward declarations for visitor. */
struct ThisConstant;
struct IntegerConstant;
struct StringConstant;
struct UnaryExpression;
struct BinaryExpression;
struct ScalarVariable;
struct VectorVariable;
struct SubroutineCall;

/** Visitor pattern for expressions. */
struct ExpressionVisitor {
    virtual ~ExpressionVisitor() = default;
    virtual void visit(ThisConstant*) = 0;
    virtual void visit(IntegerConstant*) = 0;
    virtual void visit(StringConstant*) = 0;
    virtual void visit(UnaryExpression*) = 0;
    virtual void visit(BinaryExpression*) = 0;
    virtual void visit(ScalarVariable*) = 0;
    virtual void visit(VectorVariable*) = 0;
    virtual void visit(SubroutineCall*) = 0;
};

/** Abstract base of all expressions. */
struct ExpressionBase {
    virtual ~ExpressionBase() = default;
    virtual void accept(ExpressionVisitor*) = 0;
};
typedef std::unique_ptr<ExpressionBase> Expression;
typedef std::vector<Expression> ExpressionList;

struct ThisConstant : ExpressionBase {
    virtual void accept(ExpressionVisitor* visitor) { visitor->visit(this); }
};

struct IntegerConstant : ExpressionBase {
    IntegerConstant(int value)
        : value(value)
    {
    }

    virtual void accept(ExpressionVisitor* visitor) { visitor->visit(this); }

    int value;
};

struct StringConstant : ExpressionBase {
    StringConstant(std::string value)
        : value(value)
    {
    }

    virtual void accept(ExpressionVisitor* visitor) { visitor->visit(this); }

    std::string value;
};

struct UnaryExpression : ExpressionBase {
    enum class Type { MINUS, NOT };

    UnaryExpression(Type type, Expression argument)
        : type(type)
        , argument(std::move(argument)){};

    virtual void accept(ExpressionVisitor* visitor) { visitor->visit(this); }

    Type type;
    Expression argument;
};

struct BinaryExpression : ExpressionBase {
    enum class Type { ADD, SUBSTRACT, MULTIPLY, DIVIDE, AND, OR, GREATER, LESSER, EQUAL };

    BinaryExpression(Type type, Expression argument1, Expression argument2)
        : type(type)
        , argument1(std::move(argument1))
        , argument2(std::move(argument2)){};

    virtual void accept(ExpressionVisitor* visitor) { visitor->visit(this); }

    Type type;
    Expression argument1;
    Expression argument2;
};

struct ScalarVariable : ExpressionBase {
    ScalarVariable(std::string name)
        : name(name)
    {
    }

    virtual void accept(ExpressionVisitor* visitor) { visitor->visit(this); }

    std::string name;
};

struct VectorVariable : ExpressionBase {
    VectorVariable(std::string name, Expression subscript)
        : name(name)
        , subscript(std::move(subscript))
    {
    }

    virtual void accept(ExpressionVisitor* visitor) { visitor->visit(this); }

    std::string name;
    Expression subscript;
};

struct SubroutineCall : ExpressionBase {
    SubroutineCall(std::string base, std::string name, ExpressionList arguments)
        : base(base)
        , name(name)
        , arguments(std::move(arguments))
    {
    }

    virtual void accept(ExpressionVisitor* visitor) { visitor->visit(this); }

    std::string base;
    std::string name;
    ExpressionList arguments;
};

/******************************************************************************
 *
 * Statements
 *
 *****************************************************************************/

/** Forward declarations for visitor. */
struct LetScalar;
struct LetVector;
struct IfStatement;
struct WhileStatement;
struct DoStatement;
struct ReturnStatement;

/** Visitor pattern for statements. */
struct StatementVisitor {
    virtual ~StatementVisitor() = default;
    virtual void visit(LetScalar*) = 0;
    virtual void visit(LetVector*) = 0;
    virtual void visit(IfStatement*) = 0;
    virtual void visit(WhileStatement*) = 0;
    virtual void visit(DoStatement*) = 0;
    virtual void visit(ReturnStatement*) = 0;
};

/** Abstract base of all statements. */
struct StatementBase {
    virtual ~StatementBase() = default;
    virtual void accept(StatementVisitor*) = 0;
};
typedef std::unique_ptr<StatementBase> Statement;
typedef std::vector<Statement> StatementList;

struct LetScalar : StatementBase {
    LetScalar(std::string name, Expression rvalue)
        : name(name)
        , rvalue(std::move(rvalue))
    {
    }

    virtual void accept(StatementVisitor* visitor) { visitor->visit(this); }

    std::string name;
    Expression rvalue;
};

struct LetVector : StatementBase {
    LetVector(std::string name, Expression subscript, Expression rvalue)
        : name(name)
        , subscript(std::move(subscript))
        , rvalue(std::move(rvalue))
    {
    }

    virtual void accept(StatementVisitor* visitor) { visitor->visit(this); }

    std::string name;
    Expression subscript;
    Expression rvalue;
};

struct IfStatement : StatementBase {
    IfStatement(Expression condition, StatementList positiveBranch, StatementList negativeBranch)
        : condition(std::move(condition))
        , positiveBranch(std::move(positiveBranch))
        , negativeBranch(std::move(negativeBranch))
    {
    }

    virtual void accept(StatementVisitor* visitor) { visitor->visit(this); }

    Expression condition;
    StatementList positiveBranch;
    StatementList negativeBranch;
};

struct WhileStatement : StatementBase {
    WhileStatement(Expression condition, StatementList body)
        : condition(std::move(condition))
        , body(std::move(body))
    {
    }

    virtual void accept(StatementVisitor* visitor) { visitor->visit(this); }

    Expression condition;
    StatementList body;
};

struct DoStatement : StatementBase {
    DoStatement(std::string base, std::string name, ExpressionList arguments)
        : base(base)
        , name(name)
        , arguments(std::move(arguments))
    {
    }

    virtual void accept(StatementVisitor* visitor) { visitor->visit(this); }

    std::string base;
    std::string name;
    ExpressionList arguments;
};

struct ReturnStatement : StatementBase {
    ReturnStatement(Expression value = 0)
        : value(std::move(value))
    {
    }

    virtual void accept(StatementVisitor* visitor) { visitor->visit(this); }

    Expression value;
};

/******************************************************************************
 *
 * Subroutine
 *
 *****************************************************************************/
struct Subroutine {
    enum class Kind { CONSTRUCTOR, FUNCTION, METHOD };

    Kind kind;
    VariableType returnType;
    std::string name;
    VariableDeclarationList arguments;
    VariableDeclarationList variables;
    StatementList statements;
};
typedef std::vector<Subroutine> SubroutineList;

struct Class {
    std::string name;
    VariableDeclarationList staticVariables;
    VariableDeclarationList fieldVariables;
    SubroutineList subroutines;
};

} // namespace ast {
} // namespace jack {
} // namespace hcc {
