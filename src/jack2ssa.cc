// Copyright (c) 2013 Dano Pernis
// See LICENSE.txt

#include "JackParser.h"
#include "ssa.h"
#include <stack>
#include <map>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <boost/algorithm/string/join.hpp>
#include <initializer_list>

using namespace hcc::jack;
using namespace hcc::ssa;

namespace {
struct Symbol {
    enum class Storage {
        STATIC,
        FIELD,
        ARGUMENT,
        LOCAL
    };

    Symbol(Storage storage, ast::VariableType type)
        : storage(storage)
        , type(std::move(type))
    {}

    Storage storage;
    ast::VariableType type;
    unsigned index;
};
typedef std::map<std::string, Symbol> Scope;
} // end anonymous namespace

struct Writer
    : public ast::StatementVisitor
    , public ast::ExpressionVisitor
{
    void beginBB(const std::string& label)
    { appendToCurrentBB(instruction_type::LABEL, {label}); }

    void appendToCurrentBB(instruction_type type, std::initializer_list<std::string> il)
    { appendToCurrentBB(type, std::vector<std::string>(il)); }

    void appendToCurrentBB(instruction_type type, std::vector<std::string> args)
    { currentSubroutine->second.instructions.emplace_back(type, args); }

    void outputBranch(const std::string& branch)
    { appendToCurrentBB(instruction_type::JUMP, {branch}); }

    void outputBranch(const std::string& variable, const std::string& positive, const std::string& negative)
    { appendToCurrentBB(instruction_type::BRANCH, {variable, positive, negative}); }

    std::string ssaStackTopPop()
    {
        assert(!ssaStack.empty());
        std::string top = ssaStack.top();
        ssaStack.pop();
        return top;
    }
    std::string genTempName()
    {
        return "%" + std::to_string(tmpCounter++);
    }

    /** statement visitor */

    virtual void visit(ast::LetScalar* letScalar)
    {
        letScalar->rvalue->accept(this);
        auto last = ssaStackTopPop();

        auto it = resolve(letScalar->name);
        if (!it)
            throw std::runtime_error("unresolved variable" + letScalar->name);

        switch ((*it)->second.storage) {
        case Symbol::Storage::STATIC:
            appendToCurrentBB(instruction_type::STORE, {"@" + className + "." + letScalar->name, last});
            break;
        case Symbol::Storage::FIELD: {
            const auto addr = genTempName();
            appendToCurrentBB(instruction_type::ADD, {addr, "%this", std::to_string((*it)->second.index)});
            appendToCurrentBB(instruction_type::STORE, {addr, last});
            } break;
        default:
            appendToCurrentBB(instruction_type::MOV, {"%" + letScalar->name, last});
        }

    }

    virtual void visit(ast::LetVector* letVector)
    {
        letVector->subscript->accept(this);
        auto offset = ssaStackTopPop();
        auto addr = genTempName();
        auto base = genTempName();

        auto it = resolve(letVector->name);
        if (!it)
            throw std::runtime_error("unresolved variable" + letVector->name);

        switch ((*it)->second.storage) {
        case Symbol::Storage::STATIC:
            appendToCurrentBB(instruction_type::LOAD, {base, "@" + className + "." + letVector->name});
            break;
        case Symbol::Storage::FIELD: {
            const auto _addr = genTempName();
            appendToCurrentBB(instruction_type::ADD, {_addr, "%this", std::to_string((*it)->second.index)});
            appendToCurrentBB(instruction_type::LOAD, {base, _addr});
            } break;
        default:
            appendToCurrentBB(instruction_type::MOV, {base, "%" + letVector->name});
        }

        appendToCurrentBB(instruction_type::ADD, {addr, base, offset});

        letVector->rvalue->accept(this);
        auto rvalue = ssaStackTopPop();
        appendToCurrentBB(instruction_type::STORE, {addr, rvalue});
    }

    virtual void visit(ast::IfStatement* ifStatement)
    {
        auto ifIndex = std::to_string(ifCounter++);
        auto labelTrue  = "IF_TRUE"  + ifIndex;
        auto labelFalse = "IF_FALSE" + ifIndex;
        auto labelEnd   = "IF_END"   + ifIndex;

        ifStatement->condition->accept(this);
        auto condition = ssaStackTopPop();
        outputBranch(condition, labelTrue, labelFalse);

        beginBB(labelTrue);
        for (const auto& statement : ifStatement->positiveBranch) {
            statement->accept(this);
        }
        outputBranch(labelEnd);

        beginBB(labelFalse);
        for (const auto& statement : ifStatement->negativeBranch) {
            statement->accept(this);
        }
        outputBranch(labelEnd);

        beginBB(labelEnd);
    }

    virtual void visit(ast::WhileStatement* whileStatement)
    {

        auto whileIndex = std::to_string(whileCounter++);
        auto labelCondition = "WHILE_CONDITION" + whileIndex;
        auto labelBody      = "WHILE_BODY"      + whileIndex;
        auto endBody        = "WHILE_END"       + whileIndex;

        outputBranch(labelCondition);
        beginBB(labelCondition);
        whileStatement->condition->accept(this);
        auto condition = ssaStackTopPop();
        outputBranch(condition, labelBody, endBody);

        beginBB(labelBody);
        for (const auto& statement : whileStatement->body) {
            statement->accept(this);
        }
        outputBranch(labelCondition);

        beginBB(endBody);
    }

    virtual void visit(ast::ReturnStatement* returnStatement)
    {
        if (returnStatement->value) {
            returnStatement->value->accept(this);
            auto last = ssaStackTopPop();
            appendToCurrentBB(instruction_type::RETURN, {last});
        } else {
            /* Subroutine always returns. */
            appendToCurrentBB(instruction_type::RETURN, {"0"});
        }
    }

    /** expression visitor */

    virtual void visit(ast::ThisConstant*)
    {
        ssaStack.push("%this");
    }

    virtual void visit(ast::IntegerConstant* integerConstant)
    {
        auto result = genTempName();
        ssaStack.push(result);

        appendToCurrentBB(instruction_type::MOV, {result, std::to_string(integerConstant->value)});
    }

    virtual void visit(ast::StringConstant* stringConstant)
    {
        auto string = genTempName();
        appendToCurrentBB(instruction_type::CALL, {string, "String.new", std::to_string(stringConstant->value.length())});

        for (char& c : stringConstant->value) {
            const auto string2 = genTempName();
            appendToCurrentBB(instruction_type::CALL, {string2, "String.appendChar", string, std::to_string((unsigned)c)});

            string = string2;
        }

        ssaStack.push(string);
    }

    virtual void visit(ast::UnaryExpression* unaryExpression)
    {
        unaryExpression->argument->accept(this);
        auto arg = ssaStackTopPop();

        auto result = genTempName();
        ssaStack.push(result);

        switch (unaryExpression->type) {
        case ast::UnaryExpression::Type::MINUS:
            appendToCurrentBB(instruction_type::NEG, {result, arg});
            break;
        case ast::UnaryExpression::Type::NOT:
            appendToCurrentBB(instruction_type::NOT, {result, arg});
            break;
        }
    }

    virtual void visit(ast::BinaryExpression* binaryExpression)
    {
        binaryExpression->argument1->accept(this);
        auto arg1 = ssaStackTopPop();

        binaryExpression->argument2->accept(this);
        auto arg2 = ssaStackTopPop();

        auto result = genTempName();
        ssaStack.push(result);

        switch (binaryExpression->type) {
        case ast::BinaryExpression::Type::ADD:
            appendToCurrentBB(instruction_type::ADD, {result, arg1, arg2});
            break;
        case ast::BinaryExpression::Type::SUBSTRACT:
            appendToCurrentBB(instruction_type::SUB, {result, arg1, arg2});
            break;
        case ast::BinaryExpression::Type::MULTIPLY:
            appendToCurrentBB(instruction_type::CALL, {result, "Math.multiply", arg1, arg2});
            break;
        case ast::BinaryExpression::Type::DIVIDE:
            appendToCurrentBB(instruction_type::CALL, {result, "Math.divide", arg1, arg2});
            break;
        case ast::BinaryExpression::Type::AND:
            appendToCurrentBB(instruction_type::AND, {result, arg1, arg2});
            break;
        case ast::BinaryExpression::Type::OR:
            appendToCurrentBB(instruction_type::OR, {result, arg1, arg2});
            break;
        case ast::BinaryExpression::Type::LESSER:
            appendToCurrentBB(instruction_type::LT, {result, arg1, arg2});
            break;
        case ast::BinaryExpression::Type::GREATER:
            appendToCurrentBB(instruction_type::GT, {result, arg1, arg2});
            break;
        case ast::BinaryExpression::Type::EQUAL:
            appendToCurrentBB(instruction_type::EQ, {result, arg1, arg2});
            break;
        }
    }

    virtual void visit(ast::ScalarVariable* scalarVariable)
    {
        auto result = genTempName();
        ssaStack.push(result);

        auto it = resolve(scalarVariable->name);
        if (!it)
            throw std::runtime_error("unresolved variable" + scalarVariable->name);

        switch ((*it)->second.storage) {
        case Symbol::Storage::STATIC:
            appendToCurrentBB(instruction_type::LOAD, {result, "@" + className + "." + scalarVariable->name});
            break;
        case Symbol::Storage::FIELD: {
            const auto addr = genTempName();
            appendToCurrentBB(instruction_type::ADD, {addr, "%this", std::to_string((*it)->second.index)});
            appendToCurrentBB(instruction_type::LOAD, {result, addr});
            } break;
        default:
            appendToCurrentBB(instruction_type::MOV, {result, "%" + scalarVariable->name});
        }
    }

    virtual void visit(ast::VectorVariable* vectorVariable)
    {
        vectorVariable->subscript->accept(this);
        auto offset = ssaStackTopPop();
        auto base = genTempName();
        auto addr = genTempName();

        auto it = resolve(vectorVariable->name);
        if (!it)
            throw std::runtime_error("unresolved variable" + vectorVariable->name);

        switch ((*it)->second.storage) {
        case Symbol::Storage::STATIC:
            appendToCurrentBB(instruction_type::LOAD, {base, "@" + className + "." + vectorVariable->name});
            break;
        case Symbol::Storage::FIELD: {
            const auto _addr = genTempName();
            appendToCurrentBB(instruction_type::ADD, {_addr, "%this", std::to_string((*it)->second.index)});
            appendToCurrentBB(instruction_type::LOAD, {base, _addr});
            } break;
        default:
            appendToCurrentBB(instruction_type::MOV, {base, "%" + vectorVariable->name});
        }

        appendToCurrentBB(instruction_type::ADD, {addr, base, offset});

        auto result = genTempName();
        ssaStack.push(result);
        appendToCurrentBB(instruction_type::LOAD, {result, addr});
    }

    boost::optional<Scope::iterator> resolve(const std::string& name)
    {
        auto it = classScope.find(name);
        if (it != classScope.end())
            return boost::optional<Scope::iterator>(it);

        // not found
        // try to find base in subroutine scope
        it = subroutineScope.find(name);
        if (it != subroutineScope.end())
            return boost::optional<Scope::iterator>(it);

        // not found
        // no more scopes
        return boost::optional<Scope::iterator>();
    }

    std::vector<std::string> generalCall(const std::string& base, const std::string& name, const ast::ExpressionList& arguments)
    {
        std::vector<std::string> args;

        if (base.empty()) {
            args.push_back(className + "." + name);
            args.push_back("%this");
        } else {
            auto it = resolve(base);
            if (it) {
                args.push_back(
                    dynamic_cast<ast::UnresolvedType*>(
                        (*it)->second.type->clone().get()
                    )->name + "." + name);

                switch ((*it)->second.storage) {
                case Symbol::Storage::STATIC: {
                    const auto var = genTempName();
                    appendToCurrentBB(instruction_type::LOAD, {var, "@" + className + "." + base});
                    args.push_back(var);
                    } break;
                case Symbol::Storage::FIELD: {
                    const auto addr = genTempName();
                    const auto var = genTempName();
                    appendToCurrentBB(instruction_type::ADD, {addr, "%this", std::to_string((*it)->second.index)});
                    appendToCurrentBB(instruction_type::LOAD, {var, addr});
                    args.push_back(var);
                    } break;
                default:
                    args.push_back("%" + base);
                    break;
                }
            } else {
               // not found, so it must be class name
               args.push_back(base + "." + name);
            }
        }

        for (const auto& expression : arguments) {
            expression->accept(this);
            args.push_back(ssaStackTopPop());
        }

        return args;
    }

    virtual void visit(ast::DoStatement* doStatement)
    {
        std::vector<std::string> args = generalCall(doStatement->base, doStatement->name, doStatement->arguments);

        std::vector<std::string> args2;
        args2.push_back(genTempName());
        for (auto arg : args) {
            args2.push_back(arg);
        }
        appendToCurrentBB(instruction_type::CALL, args2);
    }


    virtual void visit(ast::SubroutineCall* subroutineCall)
    {
        std::vector<std::string> args = generalCall(subroutineCall->base, subroutineCall->name, subroutineCall->arguments);

        auto result = genTempName();
        ssaStack.push(result);

        std::vector<std::string> args2;
        args2.push_back(result);
        for (auto arg : args) {
            args2.push_back(arg);
        }
        appendToCurrentBB(instruction_type::CALL, args2);
    }

    void writeSubroutine(const ast::Subroutine& subroutine)
    {
        subroutineScope.clear();
        ifCounter = 0;
        whileCounter = 0;
        tmpCounter = 0;

        currentSubroutine = res.insert_subroutine(
            className + "." + subroutine.name);

        beginBB("ENTRY");

        // arguments
        unsigned argument_counter = 0;
        if (subroutine.kind == ast::Subroutine::Kind::METHOD) {
            appendToCurrentBB(instruction_type::ARGUMENT, {"%this", std::to_string(argument_counter++)});
        }
        for (const auto& variable : subroutine.arguments) {
            subroutineScope.insert(std::make_pair(variable.name, Symbol(
                Symbol::Storage::ARGUMENT,
                variable.type->clone())));
            appendToCurrentBB(instruction_type::ARGUMENT, {"%" + variable.name, std::to_string(argument_counter++)});
        }


        // in Jack, every local variable is required to be initialized to zero
        for (const auto& variable : subroutine.variables) {
            subroutineScope.insert(std::make_pair(variable.name, Symbol(
                Symbol::Storage::LOCAL,
                variable.type->clone())));

            appendToCurrentBB(instruction_type::MOV, {"%" + variable.name, "0"});
        }

        // constructor needs to allocate space
        if (subroutine.kind == ast::Subroutine::Kind::CONSTRUCTOR) {
            appendToCurrentBB(instruction_type::CALL, {"%this", "Memory.alloc", std::to_string(fieldVarsCount)});
        }

        for (const auto& statement : subroutine.statements) {
            statement->accept(this);
        }
    }

    void write(const ast::Class& class_)
    {
        classScope.clear();
        className = class_.name;

        for (const auto& variable : class_.staticVariables) {
            classScope.insert(std::make_pair(variable.name, Symbol(
                Symbol::Storage::STATIC,
                variable.type->clone())));
            res.globals.insert(className + "." + variable.name);
        }

        fieldVarsCount = 0;
        for (const auto& variable : class_.fieldVariables) {
            classScope.insert(std::make_pair(variable.name, Symbol(
                Symbol::Storage::FIELD,
                variable.type->clone()))).first->second.index = fieldVarsCount++;
        }

        for (const auto& subroutine : class_.subroutines) {
            writeSubroutine(subroutine);
        }
    }

    unit res;

    std::string className;
    Scope classScope;
    Scope subroutineScope;
    unsigned ifCounter;
    unsigned whileCounter;
    unsigned tmpCounter;
    unsigned fieldVarsCount;
    std::stack<std::string> ssaStack;
    subroutine_map::iterator currentSubroutine;
};


int main(int argc, char* argv[])
try {
    if (argc < 2) {
        throw std::runtime_error("Missing input file(s)\n");
    }

    // parse input
    std::vector<ast::Class> classes;
    try {
        Parser parser;
        for (int i = 1; i<argc; ++i) {
            classes.push_back(parser.parse(argv[i]));
        }
    } catch (const ParseError& e) {
        std::stringstream ss;
        ss  << "Parse error: " << e.what()
            << " at " << e.line << ":" << e.column << '\n';
        throw std::runtime_error(ss.str());
    }

    // produce intermediate code
    Writer writer;
    for (const auto& class_ : classes) {
        writer.write(class_);
    }

    // optimize
    for (auto& subroutine_entry : writer.res.subroutines) {
        auto& subroutine = subroutine_entry.second;
        subroutine.construct_minimal_ssa();
        subroutine.copy_propagation();
        subroutine.dead_code_elimination();
        subroutine.ssa_deconstruct();

        unsigned int c1 = 0;
        unsigned int c2 = 0;
        subroutine.prettify_names(c1, c2);
        subroutine.clean_cfg();
    }

    // output
    writer.res.save(std::cout);

    return 0;
} catch (const std::runtime_error& e) {
    std::cerr
        << "When executing "
        << boost::algorithm::join(std::vector<std::string>(argv, argv + argc), " ")
        << " ...\n" << e.what();
    return 1;
}
