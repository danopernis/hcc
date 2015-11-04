// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "JackVMWriter.h"
#include <list>
#include <map>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <fstream>

namespace hcc {

struct Symbol {
    enum class Storage { STATIC, FIELD, ARGUMENT, LOCAL };

    Symbol(Storage storage, jack::ast::VariableType type, unsigned index)
        : storage(storage)
        , type(std::move(type))
        , index(index)
    {
    }

    Storage storage;
    jack::ast::VariableType type;
    unsigned index;
};
typedef std::map<std::string, Symbol> Scope;

} // namespace hcc {

namespace hcc {
namespace jack {

struct VMWriter::Impl : public ast::StatementVisitor, public ast::ExpressionVisitor {
    std::ofstream output;

    Impl(const std::string& filename)
        : output(filename)
    {
    }

    /** statement visitor */

    virtual void visit(ast::LetScalar* letScalar)
    {
        letScalar->rvalue->accept(this);
        output << "pop " << symbolToSegmentIndex(letScalar->name, argumentOffset) << '\n';
    }

    virtual void visit(ast::LetVector* letVector)
    {
        letVector->subscript->accept(this);
        output << "push " << symbolToSegmentIndex(letVector->name, argumentOffset) << '\n';
        output << "add\n";
        letVector->rvalue->accept(this);
        output << "pop temp 0\n"
               << "pop pointer 1\n"
               << "push temp 0\n"
               << "pop that 0\n";
    }

    virtual void visit(ast::IfStatement* ifStatement)
    {
        const unsigned ifIndex = ifCounter++;

        ifStatement->condition->accept(this);
        output << "if-goto IF_TRUE" << ifIndex << '\n' << "goto IF_FALSE" << ifIndex << '\n'
               << "label IF_TRUE" << ifIndex << '\n';
        for (const auto& statement : ifStatement->positiveBranch) {
            statement->accept(this);
        }
        if (ifStatement->negativeBranch.empty()) {
            output << "label IF_FALSE" << ifIndex << '\n';
        } else {
            output << "goto IF_END" << ifIndex << '\n' << "label IF_FALSE" << ifIndex << '\n';
            for (const auto& statement : ifStatement->negativeBranch) {
                statement->accept(this);
            }
            output << "label IF_END" << ifIndex << '\n';
        }
    }

    virtual void visit(ast::WhileStatement* whileStatement)
    {
        const unsigned whileIndex = whileCounter++;

        output << "label WHILE_EXP" << whileIndex << '\n';
        whileStatement->condition->accept(this);
        output << "not\n"
               << "if-goto WHILE_END" << whileIndex << '\n';
        for (const auto& statement : whileStatement->body) {
            statement->accept(this);
        }
        output << "goto WHILE_EXP" << whileIndex << '\n' << "label WHILE_END" << whileIndex << '\n';
    }

    virtual void visit(ast::DoStatement* doStatement)
    {
        std::string doCompoundStart;
        unsigned expressionsCount = 0;

        if (doStatement->base.empty()) {
            output << "push pointer 0\n";
            ++expressionsCount;

            doCompoundStart = className;
        } else {
            if (contains(doStatement->base)) {
                const Symbol& symbol = get(doStatement->base);
                output << "push " << symbolToSegmentIndex(doStatement->base, argumentOffset)
                       << '\n';
                ++expressionsCount;

                doCompoundStart
                    = dynamic_cast<jack::ast::UnresolvedType*>(symbol.type->clone().get())->name;
            } else {
                doCompoundStart = doStatement->base;
            }
        }

        for (const auto& expression : doStatement->arguments) {
            expression->accept(this);
            ++expressionsCount;
        }

        output << "call " << doCompoundStart << "." << doStatement->name << " " << expressionsCount
               << '\n';
        /* Do not spoil the stack!
         * Each subroutine has a return value. Had we not popped it, stack could grow
         * indefinitely if "do statement" is inside the loop */
        output << "pop temp 0\n";
    }

    virtual void visit(ast::ReturnStatement* returnStatement)
    {
        if (returnStatement->value) {
            returnStatement->value->accept(this);
        } else {
            /* Subroutine always returns. Push arbitrary value to stack */
            output << "push constant 0\n";
        }
        output << "return\n";
    }

    /** expression visitor */

    virtual void visit(ast::ThisConstant*) { output << "push pointer 0\n"; }

    virtual void visit(ast::IntegerConstant* integerConstant)
    {
        output << "push constant " << integerConstant->value << "\n";
    }

    virtual void visit(ast::StringConstant* stringConstant)
    {
        output << "push constant " << stringConstant->value.length() << '\n'
               << "call String.new 1\n";
        for (char& c : stringConstant->value) {
            output << "push constant " << (unsigned)c << '\n' << "call String.appendChar 2\n";
        }
    }

    virtual void visit(ast::UnaryExpression* unaryExpression)
    {
        unaryExpression->argument->accept(this);

        switch (unaryExpression->type) {
        case ast::UnaryExpression::Type::MINUS:
            output << "neg\n";
            break;
        case ast::UnaryExpression::Type::NOT:
            output << "not\n";
            break;
        }
    }

    virtual void visit(ast::BinaryExpression* binaryExpression)
    {
        binaryExpression->argument1->accept(this);
        binaryExpression->argument2->accept(this);

        switch (binaryExpression->type) {
        case ast::BinaryExpression::Type::ADD:
            output << "add\n";
            break;
        case ast::BinaryExpression::Type::SUBSTRACT:
            output << "sub\n";
            break;
        case ast::BinaryExpression::Type::MULTIPLY:
            output << "call Math.multiply 2\n";
            break;
        case ast::BinaryExpression::Type::DIVIDE:
            output << "call Math.divide 2\n";
            break;
        case ast::BinaryExpression::Type::AND:
            output << "and\n";
            break;
        case ast::BinaryExpression::Type::OR:
            output << "or\n";
            break;
        case ast::BinaryExpression::Type::LESSER:
            output << "lt\n";
            break;
        case ast::BinaryExpression::Type::GREATER:
            output << "gt\n";
            break;
        case ast::BinaryExpression::Type::EQUAL:
            output << "eq\n";
            break;
        }
    }

    virtual void visit(ast::ScalarVariable* scalarVariable)
    {
        output << "push " << symbolToSegmentIndex(scalarVariable->name, argumentOffset) << '\n';
    }

    virtual void visit(ast::VectorVariable* vectorVariable)
    {
        vectorVariable->subscript->accept(this);
        output << "push " << symbolToSegmentIndex(vectorVariable->name, argumentOffset) << '\n'
               << "add\n"
               << "pop pointer 1\n"
               << "push that 0\n";
    }

    virtual void visit(ast::SubroutineCall* subroutineCall)
    {
        std::string callCompoundStart;
        unsigned expressionsCount = 0;

        if (contains(subroutineCall->base)) {
            const Symbol& symbol = get(subroutineCall->base);
            output << "push " << symbolToSegmentIndex(subroutineCall->base, argumentOffset) << '\n';
            ++expressionsCount;
            callCompoundStart
                = dynamic_cast<jack::ast::UnresolvedType*>(symbol.type->clone().get())->name;
        } else {
            callCompoundStart = subroutineCall->base;
            if (callCompoundStart == "") {
                callCompoundStart = className;
            }
        }

        for (const auto& expression : subroutineCall->arguments) {
            expression->accept(this);
            ++expressionsCount;
        }

        output << "call " << callCompoundStart << "." << subroutineCall->name << " "
               << expressionsCount << '\n';
    }

    void write(const ast::Class& clazz)
    {
        className = clazz.name;
        classScope.clear();

        unsigned staticVarsCount = 0;
        for (const auto& variable : clazz.staticVariables) {
            classScope.insert(
                std::make_pair(variable.name, Symbol(hcc::Symbol::Storage::STATIC,
                                                     variable.type->clone(), staticVarsCount++)));
        }

        unsigned fieldVarsCount = 0;
        for (const auto& variable : clazz.fieldVariables) {
            classScope.insert(
                std::make_pair(variable.name, Symbol(hcc::Symbol::Storage::FIELD,
                                                     variable.type->clone(), fieldVarsCount++)));
        }

        for (const auto& subroutine : clazz.subroutines) {
            ifCounter = 0;
            whileCounter = 0;
            subroutineScope.clear();

            unsigned argumentVarsCount = 0;
            for (const auto& variable : subroutine.arguments) {
                subroutineScope.insert(std::make_pair(
                    variable.name, Symbol(hcc::Symbol::Storage::ARGUMENT, variable.type->clone(),
                                          argumentVarsCount++)));
            }

            unsigned localVarsCount = 0;
            for (const auto& variable : subroutine.variables) {
                subroutineScope.insert(std::make_pair(
                    variable.name,
                    Symbol(hcc::Symbol::Storage::LOCAL, variable.type->clone(), localVarsCount++)));
            }

            output << "function " << clazz.name << "." << subroutine.name << " " << localVarsCount
                   << '\n';
            switch (subroutine.kind) {
            case ast::Subroutine::Kind::CONSTRUCTOR:
                output << "push constant " << fieldVarsCount << '\n' << "call Memory.alloc 1\n"
                       << "pop pointer 0\n";
                argumentOffset = 0;
                break;
            case ast::Subroutine::Kind::METHOD:
                output << "push argument 0\n"
                       << "pop pointer 0\n";
                argumentOffset = 1;
                break;
            case ast::Subroutine::Kind::FUNCTION:
                argumentOffset = 0;
                break;
            }

            for (const auto& statement : subroutine.statements) {
                statement->accept(this);
            }
        }
    }

    unsigned ifCounter;
    unsigned whileCounter;

    unsigned argumentOffset;

    Scope classScope;
    Scope subroutineScope;

    bool contains(const std::string& key) const
    {
        if (classScope.find(key) != classScope.end()) {
            return true;
        }
        if (subroutineScope.find(key) != subroutineScope.end()) {
            return true;
        }
        return false;
    }

    const Symbol& get(const std::string& key) const
    {
        auto subroutineScopeResult = subroutineScope.find(key);
        if (subroutineScopeResult != subroutineScope.end()) {
            return subroutineScopeResult->second;
        }
        auto classScopeResult = classScope.find(key);
        if (classScopeResult != classScope.end()) {
            return classScopeResult->second;
        }
        assert(false);
    }

    std::string symbolToSegmentIndex(std::string name, int argumentOffset) const
    {
        const auto& symbol = get(name);

        // throw std::runtime_error("no such variable: " + name);

        std::stringstream result;

        switch (symbol.storage) {
        case hcc::Symbol::Storage::STATIC:
            result << "static " << symbol.index;
            break;
        case hcc::Symbol::Storage::FIELD:
            result << "this " << symbol.index;
            break;
        case hcc::Symbol::Storage::ARGUMENT:
            result << "argument " << (symbol.index + argumentOffset);
            break;
        case hcc::Symbol::Storage::LOCAL:
            result << "local " << symbol.index;
            break;
        }
        return result.str();
    }

    std::string className;
};

void VMWriter::write(const ast::Class& clazz) { pimpl->write(clazz); }

VMWriter::VMWriter(const std::string& filename)
    : pimpl(new Impl(filename))
{
}

// this needs to be in implementation file for pimpl + unique to work
VMWriter::~VMWriter() = default;

} // namespace jack {
} // namespace hcc {
