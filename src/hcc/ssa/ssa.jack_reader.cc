// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/ssa/ssa.h"

#include "hcc/jack/ast.h"
#include "hcc/ssa/subroutine_builder.h"

#include <boost/optional.hpp>
#include <cassert>
#include <initializer_list>
#include <stack>
#include <stdexcept>

namespace hcc {
namespace ssa {

using namespace hcc::jack;

namespace {

enum class storage_type { STATIC, FIELD, ARGUMENT, LOCAL };

struct symbol {
    symbol(storage_type storage, VariableType type)
        : storage(std::move(storage))
        , type(std::move(type))
    {
    }

    storage_type storage;
    VariableType type;
    unsigned index;
};

using scope = std::map<std::string, symbol>;

struct scopes {
    boost::optional<scope::const_iterator> resolve(const std::string& name)
    {
        for (const auto& scope : scopes) {
            const auto it = scope.find(name);
            if (it != scope.end()) {
                return it;
            }
        }
        return boost::none;
    }

    std::vector<scope> scopes;
};

struct writer : public StatementVisitor, public ExpressionVisitor {
    void open_block(const std::string& label) { block = builder.add_bb(label); }

    void write_instruction(instruction_type type, std::initializer_list<argument> il)
    {
        write_instruction(type, std::vector<argument>(il));
    }

    void write_instruction(instruction_type type, std::vector<argument> args)
    {
        builder.add_instruction(block, instruction(type, args));
    }

    void close_block(const std::string& branch) { builder.add_jump(block, builder.add_bb(branch)); }

    void close_block(const argument& variable, const std::string& positive,
                     const std::string& negative)
    {
        builder.add_branch(block, instruction_type::JEQ, constant(0), variable,
                           builder.add_bb(negative), builder.add_bb(positive));
    }

    void write_return(const argument& variable) { builder.add_return(block, variable); }

    argument ssa_stack_top_pop()
    {
        assert(!ssa_stack.empty());
        auto top = ssa_stack.top();
        ssa_stack.pop();
        return top;
    }
    argument temporary() { return builder.add_reg(std::to_string(temporary_counter++)); }

    /** statement visitor */

    virtual void visit(LetScalar* letScalar)
    {
        letScalar->rvalue->accept(this);
        const auto last = ssa_stack_top_pop();

        const auto it = symbols.resolve(letScalar->name);
        if (!it) {
            throw std::runtime_error("unresolved variable" + letScalar->name);
        }

        switch ((*it)->second.storage) {
        case storage_type::STATIC:
            write_instruction(instruction_type::STORE,
                              {res.globals.put(prefix + letScalar->name), last});
            break;
        case storage_type::FIELD: {
            const auto addr = temporary();
            write_instruction(instruction_type::ADD,
                              {addr, builder.add_reg("this"), constant((*it)->second.index)});
            write_instruction(instruction_type::STORE, {addr, last});
        } break;
        default:
            write_instruction(instruction_type::MOV, {builder.add_reg(letScalar->name), last});
        }
    }

    virtual void visit(LetVector* letVector)
    {
        letVector->subscript->accept(this);
        const auto offset = ssa_stack_top_pop();
        const auto addr = temporary();
        const auto base = temporary();

        const auto it = symbols.resolve(letVector->name);
        if (!it) {
            throw std::runtime_error("unresolved variable" + letVector->name);
        }

        switch ((*it)->second.storage) {
        case storage_type::STATIC:
            write_instruction(instruction_type::LOAD,
                              {base, res.globals.put(prefix + letVector->name)});
            break;
        case storage_type::FIELD: {
            const auto _addr = temporary();
            write_instruction(instruction_type::ADD,
                              {_addr, builder.add_reg("this"), constant((*it)->second.index)});
            write_instruction(instruction_type::LOAD, {base, _addr});
        } break;
        default:
            write_instruction(instruction_type::MOV, {base, builder.add_reg(letVector->name)});
        }

        write_instruction(instruction_type::ADD, {addr, base, offset});

        letVector->rvalue->accept(this);
        const auto rvalue = ssa_stack_top_pop();
        write_instruction(instruction_type::STORE, {addr, rvalue});
    }

    virtual void visit(IfStatement* ifStatement)
    {
        const auto if_index = std::to_string(if_counter++);
        const auto label_true = "IF_TRUE" + if_index;
        const auto label_false = "IF_FALSE" + if_index;
        const auto label_end = "IF_END" + if_index;

        ifStatement->condition->accept(this);
        const auto condition = ssa_stack_top_pop();
        close_block(condition, label_true, label_false);

        open_block(label_true);
        for (const auto& statement : ifStatement->positiveBranch) {
            statement->accept(this);
        }
        close_block(label_end);

        open_block(label_false);
        for (const auto& statement : ifStatement->negativeBranch) {
            statement->accept(this);
        }
        close_block(label_end);

        open_block(label_end);
    }

    virtual void visit(WhileStatement* whileStatement)
    {
        const auto while_index = std::to_string(while_counter++);
        const auto label_condition = "WHILE_CONDITION" + while_index;
        const auto label_body = "WHILE_BODY" + while_index;
        const auto label_end = "WHILE_END" + while_index;

        close_block(label_condition);
        open_block(label_condition);
        whileStatement->condition->accept(this);
        const auto condition = ssa_stack_top_pop();
        close_block(condition, label_body, label_end);

        open_block(label_body);
        for (const auto& statement : whileStatement->body) {
            statement->accept(this);
        }
        close_block(label_condition);

        open_block(label_end);
    }

    virtual void visit(ReturnStatement* returnStatement)
    {
        if (returnStatement->value) {
            returnStatement->value->accept(this);
            const auto last = ssa_stack_top_pop();
            write_return(last);
        } else {
            write_return(constant(0));
        }
        // Each block must end with exactly one terminator, so we have to create
        // another block.
        open_block("DUMMY" + std::to_string(dummy_counter++));
    }

    /** expression visitor */

    virtual void visit(ThisConstant*) { ssa_stack.push(builder.add_reg("this")); }

    virtual void visit(IntegerConstant* integerConstant)
    {
        const auto result = temporary();
        ssa_stack.push(result);

        write_instruction(instruction_type::MOV, {result, constant(integerConstant->value)});
    }

    virtual void visit(StringConstant* stringConstant)
    {
        auto string = temporary();
        write_instruction(instruction_type::CALL, {string, res.globals.put("String.new"),
                                                   constant(stringConstant->value.length())});

        for (char& c : stringConstant->value) {
            const auto string2 = temporary();
            write_instruction(
                instruction_type::CALL,
                {string2, res.globals.put("String.appendChar"), string, constant((unsigned)c)});

            string = string2;
        }

        ssa_stack.push(string);
    }

    virtual void visit(UnaryExpression* unaryExpression)
    {
        unaryExpression->argument->accept(this);
        const auto arg = ssa_stack_top_pop();

        const auto result = temporary();
        ssa_stack.push(result);

        switch (unaryExpression->type) {
        case UnaryExpression::Type::MINUS:
            write_instruction(instruction_type::NEG, {result, arg});
            break;
        case UnaryExpression::Type::NOT:
            write_instruction(instruction_type::NOT, {result, arg});
            break;
        }
    }

    void write_compare(const instruction_type& it, const argument& result, const argument& arg1,
                       const argument& arg2)
    {
        const auto compare_index = std::to_string(compare_counter++);
        const auto label_true = "COMPARE_TRUE" + compare_index;
        const auto label_end = "COMPARE_END" + compare_index;

        write_instruction(instruction_type::MOV, {result, constant(0)});
        builder.add_branch(block, it, arg1, arg2, builder.add_bb(label_true),
                           builder.add_bb(label_end));
        open_block(label_true);
        write_instruction(instruction_type::MOV, {result, constant(-1)});
        close_block(label_end);
        open_block(label_end);
    }

    virtual void visit(BinaryExpression* binaryExpression)
    {
        binaryExpression->argument1->accept(this);
        const auto arg1 = ssa_stack_top_pop();

        binaryExpression->argument2->accept(this);
        const auto arg2 = ssa_stack_top_pop();

        const auto result = temporary();
        ssa_stack.push(result);

        switch (binaryExpression->type) {
        case BinaryExpression::Type::ADD:
            write_instruction(instruction_type::ADD, {result, arg1, arg2});
            break;
        case BinaryExpression::Type::SUBSTRACT:
            write_instruction(instruction_type::SUB, {result, arg1, arg2});
            break;
        case BinaryExpression::Type::MULTIPLY:
            write_instruction(instruction_type::CALL,
                              {result, res.globals.put("Math.multiply"), arg1, arg2});
            break;
        case BinaryExpression::Type::DIVIDE:
            write_instruction(instruction_type::CALL,
                              {result, res.globals.put("Math.divide"), arg1, arg2});
            break;
        case BinaryExpression::Type::AND:
            write_instruction(instruction_type::AND, {result, arg1, arg2});
            break;
        case BinaryExpression::Type::OR:
            write_instruction(instruction_type::OR, {result, arg1, arg2});
            break;
        case BinaryExpression::Type::LESSER:
            write_compare(instruction_type::JLT, result, arg1, arg2);
            break;
        case BinaryExpression::Type::GREATER:
            write_compare(instruction_type::JLT, result, arg2, arg1);
            break;
        case BinaryExpression::Type::EQUAL:
            write_compare(instruction_type::JEQ, result, arg1, arg2);
            break;
        }
    }

    virtual void visit(ScalarVariable* scalarVariable)
    {
        const auto result = temporary();
        ssa_stack.push(result);

        const auto it = symbols.resolve(scalarVariable->name);
        if (!it) {
            throw std::runtime_error("unresolved variable" + scalarVariable->name);
        }

        switch ((*it)->second.storage) {
        case storage_type::STATIC:
            write_instruction(instruction_type::LOAD,
                              {result, res.globals.put(prefix + scalarVariable->name)});
            break;
        case storage_type::FIELD: {
            const auto addr = temporary();
            write_instruction(instruction_type::ADD,
                              {addr, builder.add_reg("this"), constant((*it)->second.index)});
            write_instruction(instruction_type::LOAD, {result, addr});
        } break;
        default:
            write_instruction(instruction_type::MOV,
                              {result, builder.add_reg(scalarVariable->name)});
        }
    }

    virtual void visit(VectorVariable* vectorVariable)
    {
        vectorVariable->subscript->accept(this);
        const auto offset = ssa_stack_top_pop();
        const auto base = temporary();
        const auto addr = temporary();

        const auto it = symbols.resolve(vectorVariable->name);
        if (!it) {
            throw std::runtime_error("unresolved variable" + vectorVariable->name);
        }

        switch ((*it)->second.storage) {
        case storage_type::STATIC:
            write_instruction(instruction_type::LOAD,
                              {base, res.globals.put(prefix + vectorVariable->name)});
            break;
        case storage_type::FIELD: {
            const auto _addr = temporary();
            write_instruction(instruction_type::ADD,
                              {_addr, builder.add_reg("this"), constant((*it)->second.index)});
            write_instruction(instruction_type::LOAD, {base, _addr});
        } break;
        default:
            write_instruction(instruction_type::MOV, {base, builder.add_reg(vectorVariable->name)});
        }

        write_instruction(instruction_type::ADD, {addr, base, offset});

        const auto result = temporary();
        ssa_stack.push(result);
        write_instruction(instruction_type::LOAD, {result, addr});
    }

    argument write_call(const std::string& base, const std::string& name,
                        const ExpressionList& arguments)
    {
        const auto result = temporary();

        std::vector<argument> args;
        args.push_back(result);

        if (base.empty()) {
            args.push_back(res.globals.put(prefix + name));
            args.push_back(builder.add_reg("this"));
        } else {
            const auto it = symbols.resolve(base);
            if (it) {
                args.push_back(
                    res.globals.put(dynamic_cast<UnresolvedType*>((*it)->second.type->clone().get())
                                        ->name + "." + name));

                switch ((*it)->second.storage) {
                case storage_type::STATIC: {
                    const auto var = temporary();
                    write_instruction(instruction_type::LOAD,
                                      {var, res.globals.put(prefix + base)});
                    args.push_back(var);
                } break;
                case storage_type::FIELD: {
                    const auto addr = temporary();
                    const auto var = temporary();
                    write_instruction(instruction_type::ADD, {addr, builder.add_reg("this"),
                                                              constant((*it)->second.index)});
                    write_instruction(instruction_type::LOAD, {var, addr});
                    args.push_back(var);
                } break;
                default:
                    args.push_back(builder.add_reg(base));
                    break;
                }
            } else {
                // not found, so it must be class name
                args.push_back(res.globals.put(base + "." + name));
            }
        }

        for (const auto& expression : arguments) {
            expression->accept(this);
            args.push_back(ssa_stack_top_pop());
        }

        write_instruction(instruction_type::CALL, args);
        return result;
    }

    virtual void visit(DoStatement* doStatement)
    {
        write_call(doStatement->base, doStatement->name, doStatement->arguments);
    }

    virtual void visit(SubroutineCall* subroutineCall)
    {
        ssa_stack.push(
            write_call(subroutineCall->base, subroutineCall->name, subroutineCall->arguments));
    }

    writer(unit& res, const Class& class_, const Subroutine& subroutine)
        : res(res)
        , prefix{class_.name + "."}
        , subroutine_iterator{res.insert_subroutine(res.globals.put(prefix + subroutine.name))}
        , builder(subroutine_iterator->second)
        , block(builder.add_bb("ENTRY", true))
    {
        symbols.scopes.emplace_back();

        for (const auto& variable : class_.staticVariables) {
            symbols.scopes.back().emplace(variable.name,
                                          symbol(storage_type::STATIC, variable.type->clone()));
        }

        for (const auto& variable : class_.fieldVariables) {
            symbols.scopes.back()
                .emplace(variable.name, symbol(storage_type::FIELD, variable.type->clone()))
                .first->second.index = field_vars_count++;
        }

        symbols.scopes.emplace_back();

        // arguments
        unsigned argument_counter = 0;
        if (subroutine.kind == Subroutine::Kind::METHOD) {
            write_instruction(instruction_type::ARGUMENT,
                              {builder.add_reg("this"), constant(argument_counter++)});
        }
        for (const auto& variable : subroutine.arguments) {
            symbols.scopes.back().emplace(variable.name,
                                          symbol(storage_type::ARGUMENT, variable.type->clone()));
            write_instruction(instruction_type::ARGUMENT,
                              {builder.add_reg(variable.name), constant(argument_counter++)});
        }

        // in Jack, every local variable is required to be initialized to zero
        for (const auto& variable : subroutine.variables) {
            symbols.scopes.back().emplace(variable.name,
                                          symbol(storage_type::LOCAL, variable.type->clone()));
            write_instruction(instruction_type::MOV, {builder.add_reg(variable.name), constant(0)});
        }

        // constructor needs to allocate space
        if (subroutine.kind == Subroutine::Kind::CONSTRUCTOR) {
            write_instruction(instruction_type::CALL,
                              {builder.add_reg("this"), res.globals.put("Memory.alloc"),
                               constant(field_vars_count)});
        }

        for (const auto& statement : subroutine.statements) {
            statement->accept(this);
        }

        // Each block must end with exactly one terminator, so we have to
        // insert terminator here.
        write_return(constant(0));

        subroutine_iterator->second.construct_minimal_ssa();
    }

    unit& res;
    std::string prefix;
    subroutine_map::iterator subroutine_iterator;
    unsigned field_vars_count = 0;
    scopes symbols;
    unsigned if_counter = 0;
    unsigned while_counter = 0;
    unsigned temporary_counter = 0;
    unsigned compare_counter = 0;
    unsigned dummy_counter = 0;
    std::stack<argument> ssa_stack;
    subroutine_builder builder;
    label block;
};

} // namespace {

void unit::translate_from_jack(const Class& class_)
{
    for (const auto& subroutine : class_.subroutines) {
        writer w{*this, class_, subroutine};
    }
}

} // namespace ssa {
} // namespace hcc {
