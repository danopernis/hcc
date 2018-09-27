// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#pragma once

#include "hcc/cpu/instruction.h"

#include <string>
#include <list>
#include <iostream>

namespace hcc {
namespace vm {

typedef enum { LOCAL, ARGUMENT, THIS, THAT, POINTER, TEMP, STATIC } Segment;
typedef enum { NEG, NOT, ADDC, SUBC, BUSC, ANDC, ORC, DOUBLE } UnaryOperation;
typedef enum { ADD, SUB, BUS, AND, OR } BinaryOperation;

struct CompareOperation {
    bool lt, eq, gt;

    void set(bool lt, bool eq, bool gt)
    {
        this->lt = lt;
        this->eq = eq;
        this->gt = gt;
    }
    CompareOperation& negate()
    {
        lt ^= true;
        eq ^= true;
        gt ^= true;

        return (*this);
    }
    CompareOperation& swap()
    {
        lt ^= true;
        gt ^= true;

        return (*this);
    }
    unsigned short jump()
    {
        if (lt) {
            if (eq)
                return gt ? instruction::JMP : instruction::JLE;
            else
                return gt ? instruction::JNE : instruction::JLT;
        } else {
            if (eq)
                return gt ? instruction::JGE : instruction::JEQ;
            else
                return gt ? instruction::JGT : 0;
        }
    }
};

class command {
public:
    typedef enum {
        NOP,
        CONSTANT,
        UNARY,
        BINARY,
        COMPARE,
        PUSH,
        POP_DIRECT,
        POP_INDIRECT,
        COPY,
        LABEL,
        GOTO,
        IF,
        UNARY_COMPARE,
        UNARY_COMPARE_IF,
        COMPARE_IF,
        FUNCTION,
        RETURN,
        CALL,
        IN,
        FIN,
        POP_INDIRECT_PUSH
    } Type;

    std::string arg1;
    Type type;
    UnaryOperation unary;
    BinaryOperation binary;
    CompareOperation compare;
    Segment segment1, segment2;
    int int1, int2;

    bool in, fin;
};

typedef std::list<command> command_list;

std::ostream& operator<<(std::ostream& out, const command& c);

} // namespace vm {
} // namespace hcc {
