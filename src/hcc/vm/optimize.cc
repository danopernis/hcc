// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/vm/optimize.h"

#include "hcc/cpu/cpu.h"
#include <iostream>
#include <stdexcept>

namespace hcc {
namespace vm {

typedef void (*o1cb)(command_list& cmds, command_list::iterator& c1);
typedef bool (*o2cb)(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2);
typedef bool (*o3cb)(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2,
                     command_list::iterator& c3);

/*
 * optimizeN routine calls callback for all successive N-tuples. If callback returns true, routine
 * restarts.
 */
void optimize1(command_list& cmds, o1cb cb)
{
    for (command_list::iterator i = cmds.begin(); i != cmds.end();) {
        command_list::iterator tmp = i;
        ++tmp;
        cb(cmds, i);
        i = tmp;
    }
}
void optimize2(command_list& cmds, o2cb cb)
{
    bool modified;
    do {
        modified = false;
        if (cmds.size() < 2)
            break;

        command_list::iterator c1, c2;
        c1 = c2 = cmds.begin();
        ++c2;

        while (c2 != cmds.end()) {
            if (cb(cmds, c1, c2)) {
                modified = true;
                break;
            }
            c1 = c2;
            ++c2;
        }
    } while (modified);
}
void optimize3(command_list& cmds, o3cb cb)
{
    bool modified;
    do {
        modified = false;
        if (cmds.size() < 3)
            break;

        command_list::iterator c1, c2, c3;
        c1 = c2 = cmds.begin();
        c3 = ++c2;
        ++c3;
        while (c3 != cmds.end()) {
            if (cb(cmds, c1, c2, c3)) {
                modified = true;
                break;
            }
            c1 = c2;
            c2 = c3;
            ++c3;
        }
    } while (modified);
}

/*
 * optimizations removing commands
 */
bool o_bloated_goto(command_list&, command_list::iterator& c1, command_list::iterator& c2,
                    command_list::iterator& c3)
{
    if (c1->type == command::IF && c2->type == command::GOTO && c3->type == command::LABEL
        && c1->arg1 == c3->arg1) {
        c1->type = command::UNARY;
        c1->unary = NOT;
        c2->type = command::IF;
        return true;
    }
    return false;
}
bool o_double_notneg(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2)
{
    if (c1->type == command::UNARY && c2->type == command::UNARY
        && ((c1->unary == NOT && c2->unary == NOT) || (c1->unary == NEG && c2->unary == NEG))) {
        cmds.erase(c1);
        cmds.erase(c2);
        return true;
    }
    return false;
}
bool o_negated_compare(command_list& cmds, command_list::iterator& c1,
                       command_list::iterator& c2)
{
    if (c1->type != command::COMPARE || c2->type != command::UNARY || c2->unary != NOT) {
        return false;
    }

    c1->compare.negate();
    cmds.erase(c2);
    return true;
}
bool o_negated_if(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2)
{
    if (c1->type != command::UNARY || c1->unary != NOT || c2->type != command::IF) {
        return false;
    }

    c2->compare.negate();
    cmds.erase(c1);
    return true;
}
/*
 * const expressions
 */
bool o_const_expression3(command_list& cmds, command_list::iterator& c1,
                         command_list::iterator& c2, command_list::iterator& c3)
{
    if (c1->type == command::CONSTANT && c2->type == command::CONSTANT) {
        if (c3->type == command::BINARY) {
            switch (c3->binary) {
            case ADD:
                c1->int1 += c2->int1;
                break;
            case BUS:
                c1->int1 = c2->int1 - c1->int1;
                break;
            case SUB:
                c1->int1 -= c2->int1;
                break;
            case AND:
                break;
            case OR:
                c1->int1 |= c2->int1;
                break;
            }
            cmds.erase(c2);
            cmds.erase(c3);
            return true;
        }
        if (c3->type == command::COMPARE) {
            bool zr, ng;
            unsigned short out;
            cpu::comp(instruction::COMP_D_MINUS_A, c1->int1, c2->int1, out, zr, ng);
            c1->int1 = cpu::jump(c3->compare.jump(), zr, ng) ? -1 : 0;
            cmds.erase(c2);
            cmds.erase(c3);
            return true;
        }
    }
    return false;
}
bool o_const_expression2(command_list& cmds, command_list::iterator& c1,
                         command_list::iterator& c2)
{
    if (c1->type == command::CONSTANT && c2->type == command::UNARY) {
        switch (c2->unary) {
        case NEG:
            c1->int1 = -c1->int1;
            break;
        case NOT:
            c1->int1 = c1->int1 ? 0 : -1;
            break;
        default:
            throw std::runtime_error("Wrong order of optimizations");
            break;
        }
        cmds.erase(c2);
        return true;
    }
    return false;
}
bool o_const_if(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2)
{
    if (c1->type != command::CONSTANT || c2->type != command::IF) {
        return false;
    }

    bool result = false;
    if (c2->compare.lt) {
        result |= (c1->int1 < 0);
    }
    if (c2->compare.eq) {
        result |= (c1->int1 == 0);
    }
    if (c2->compare.gt) {
        result |= (c1->int1 > 0);
    }

    if (result) {
        c2->type = command::GOTO;
    } else {
        cmds.erase(c2);
    }
    cmds.erase(c1);
    return true;
}
/*
 * convert binary operation to unary
 */
bool o_const_swap(command_list&, command_list::iterator& c1, command_list::iterator& c2,
                  command_list::iterator& c3)
{
    if (c1->type == command::CONSTANT && c2->type == command::PUSH) {
        if (c3->type == command::BINARY) {
            switch (c3->binary) {
            case SUB:
                c3->binary = BUS;
                break;
            case BUS:
                c3->binary = SUB;
                break;
            case ADD:
            case AND:
            case OR:
                // symmetric operations
                break;
            }
        }
        if (c3->type == command::COMPARE) {
            c3->compare.swap();
        }
        if (c3->type == command::BINARY || c3->type == command::COMPARE) {
            c1->segment1 = c2->segment1;
            c1->type = command::PUSH;
            c2->type = command::CONSTANT;
            int temp = c1->int1;
            c1->int1 = c2->int1;
            c2->int1 = temp;
            return true;
        }
    }
    return false;
}
bool o_binary_to_unary(command_list& cmds, command_list::iterator& c1,
                       command_list::iterator& c2)
{
    if (c1->type == command::CONSTANT) {
        if (c2->type == command::BINARY) {
            c2->type = command::UNARY;
            c2->int1 = c1->int1;
            switch (c2->binary) {
            case ADD:
                c2->unary = ADDC;
                break;
            case SUB:
                c2->unary = SUBC;
                break;
            case BUS:
                c2->unary = BUSC;
                break;
            case AND:
                c2->unary = ANDC;
                break;
            case OR:
                c2->unary = ORC;
                break;
            }
            cmds.erase(c1);
            return true;
        }
        if (c2->type == command::COMPARE) {
            c2->type = command::UNARY_COMPARE;
            c2->int1 = c1->int1;
            cmds.erase(c1);
            return true;
        }
    }
    return false;
}
void o_special_unary(command_list& cmds, command_list::iterator& c1)
{
    if (c1->type != command::UNARY)
        return;

    switch (c1->unary) {
    case NEG:
    case NOT:
    case DOUBLE:
        // not applicable
        break;
    case ANDC:
        // TODO
        break;
    case ADDC:
    case SUBC:
    case ORC:
        if (c1->int1 == 0) {
            cmds.erase(c1); // stack is fine, because operation is unary
        }
        break;
    case BUSC:
        if (c1->int1 == 0) {
            std::cout << "O: unary operation x -> 0-x ===> x -> -x\n";
            c1->unary = NEG;
        }
        break;
    }
}
bool o_binary_equalarg(command_list& cmds, command_list::iterator& c1,
                       command_list::iterator& c2, command_list::iterator& c3)
{
    if (c1->type == command::PUSH && c2->type == command::PUSH && c1->segment1 == c2->segment1
        && c1->int1 == c2->int1) {
        if (c3->type == command::BINARY && c3->binary == ADD) {
            c3->type = command::UNARY;
            c3->unary = DOUBLE;
            cmds.erase(c1);
            return true;
        }
        // TODO: other variants
    }
    return false;
}
/*
 * Merge operations.
 */
bool o_push_pop(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2)
{
    if (c1->type != command::PUSH || c2->type != command::POP_INDIRECT) {
        return false;
    }

    c1->type = command::COPY;
    c1->int2 = c2->int1;
    c1->segment2 = c2->segment1;
    cmds.erase(c2);
    return true;
}
bool o_compare_if(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2)
{
    if (c2->type == command::IF) {
        if (c1->type == command::COMPARE) {
            c1->type = command::COMPARE_IF;
            c1->arg1 = c2->arg1;
            cmds.erase(c2);
            return true;
        }
        if (c1->type == command::UNARY_COMPARE) {
            c1->type = command::UNARY_COMPARE_IF;
            c1->arg1 = c2->arg1;
            cmds.erase(c2);
            return true;
        }
    }
    return false;
}
bool o_goto_goto(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2)
{
    if (c1->type != command::GOTO || c2->type != command::GOTO) {
        return false;
    }

    cmds.erase(c2);
    return true;
}
bool o_pop_push(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2)
{
    if (c1->type == command::POP_INDIRECT && c2->type == command::PUSH
        && c1->segment1 == c2->segment1 && c1->int1 == c2->int1) {
        c1->type = command::POP_INDIRECT_PUSH;
        cmds.erase(c2);
        return true;
    }
    return false;
}
/*
 * stack-less computation chains
 */
void s_replicate(command_list& cmds, command_list::iterator& c1)
{
    command in;
    in.type = command::IN;

    switch (c1->type) {
    case command::UNARY:
    case command::BINARY:
    case command::COMPARE:
    case command::UNARY_COMPARE:
    case command::IF:
    case command::COMPARE_IF:
    case command::UNARY_COMPARE_IF:
    case command::POP_DIRECT:
        c1->in = false;
        cmds.insert(c1, in);
        break;
    case command::PUSH:
    case command::POP_INDIRECT_PUSH:
    case command::CONSTANT:
    default:
        break;
    }
    switch (c1->type) {
    case command::UNARY:
    case command::BINARY:
    case command::COMPARE:
    case command::UNARY_COMPARE:
    case command::PUSH:
    case command::CONSTANT:
    case command::POP_INDIRECT_PUSH:
        c1->fin = false;
        cmds.insert(c1, *c1);
        c1->type = command::FIN;
        break;
    case command::IF:
    case command::COMPARE_IF:
    case command::UNARY_COMPARE_IF:
    case command::POP_DIRECT:
    default:
        break;
    }
}
bool s_reduce(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2)
{
    if (c1->type != command::FIN || c2->type != command::IN) {
        return false;
    }

    cmds.erase(c1);
    cmds.erase(c2);
    return true;
}
bool s_reconstruct(command_list& cmds, command_list::iterator& c1, command_list::iterator& c2)
{
    if (c1->type == command::IN) {
        c2->in = true;
        cmds.erase(c1);
        return true;
    }
    if (c2->type == command::FIN) {
        c1->fin = true;
        cmds.erase(c2);
        return true;
    }
    return false;
}

void optimize(command_list& cmds)
{
    // order is somewhat important!
    // optimizations removing commands are best taken early
    optimize3(cmds, o_bloated_goto);
    optimize2(cmds, o_double_notneg);
    optimize2(cmds, o_negated_compare);
    optimize2(cmds, o_negated_if); // take this *after* o_bloated_goto

    // const expressions
    optimize3(cmds, o_const_expression3);
    optimize2(cmds, o_const_expression2);
    optimize2(cmds, o_const_if);

    // convert binary operation to unary
    optimize3(cmds, o_const_swap);
    optimize2(cmds, o_binary_to_unary);
    optimize1(cmds, o_special_unary);
    optimize3(cmds, o_binary_equalarg);

    // merge
    optimize2(cmds, o_push_pop);
    optimize2(cmds, o_compare_if);
    optimize2(cmds, o_goto_goto);
    optimize2(cmds, o_pop_push);

    // stack-less computation chain -- do NOT change order!
    optimize1(cmds, s_replicate);
    optimize2(cmds, s_reduce);
    optimize2(cmds, s_reconstruct);
}

} // namespace vm {
} // namespace hcc {
