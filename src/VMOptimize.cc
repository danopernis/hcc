// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include <iostream>
#include <stdexcept>
#include "VMOptimize.h"
#include "CPU.h"

namespace hcc {

typedef void (*o1cb)(VMCommandList& cmds, VMCommandList::iterator& c1);
typedef bool (*o2cb)(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2);
typedef bool (*o3cb)(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2,
                     VMCommandList::iterator& c3);

/*
 * optimizeN routine calls callback for all successive N-tuples. If callback returns true, routine
 * restarts.
 */
void optimize1(VMCommandList& cmds, o1cb cb)
{
    for (VMCommandList::iterator i = cmds.begin(); i != cmds.end();) {
        VMCommandList::iterator tmp = i;
        ++tmp;
        cb(cmds, i);
        i = tmp;
    }
}
void optimize2(VMCommandList& cmds, o2cb cb)
{
    bool modified;
    do {
        modified = false;
        if (cmds.size() < 2)
            break;

        VMCommandList::iterator c1, c2;
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
void optimize3(VMCommandList& cmds, o3cb cb)
{
    bool modified;
    do {
        modified = false;
        if (cmds.size() < 3)
            break;

        VMCommandList::iterator c1, c2, c3;
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
bool o_bloated_goto(VMCommandList&, VMCommandList::iterator& c1, VMCommandList::iterator& c2,
                    VMCommandList::iterator& c3)
{
    if (c1->type == VMCommand::IF && c2->type == VMCommand::GOTO && c3->type == VMCommand::LABEL
        && c1->arg1 == c3->arg1) {
        c1->type = VMCommand::UNARY;
        c1->unary = NOT;
        c2->type = VMCommand::IF;
        return true;
    }
    return false;
}
bool o_double_notneg(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2)
{
    if (c1->type == VMCommand::UNARY && c2->type == VMCommand::UNARY
        && ((c1->unary == NOT && c2->unary == NOT) || (c1->unary == NEG && c2->unary == NEG))) {
        cmds.erase(c1);
        cmds.erase(c2);
        return true;
    }
    return false;
}
bool o_negated_compare(VMCommandList& cmds, VMCommandList::iterator& c1,
                       VMCommandList::iterator& c2)
{
    if (c1->type != VMCommand::COMPARE || c2->type != VMCommand::UNARY || c2->unary != NOT) {
        return false;
    }

    c1->compare.negate();
    cmds.erase(c2);
    return true;
}
bool o_negated_if(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2)
{
    if (c1->type != VMCommand::UNARY || c1->unary != NOT || c2->type != VMCommand::IF) {
        return false;
    }

    c2->compare.negate();
    cmds.erase(c1);
    return true;
}
/*
 * const expressions
 */
bool o_const_expression3(VMCommandList& cmds, VMCommandList::iterator& c1,
                         VMCommandList::iterator& c2, VMCommandList::iterator& c3)
{
    if (c1->type == VMCommand::CONSTANT && c2->type == VMCommand::CONSTANT) {
        if (c3->type == VMCommand::BINARY) {
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
        if (c3->type == VMCommand::COMPARE) {
            bool zr, ng;
            unsigned short out;
            CPU::comp(instruction::COMP_D_MINUS_A, c1->int1, c2->int1, out, zr, ng);
            c1->int1 = CPU::jump(c3->compare.jump(), zr, ng) ? -1 : 0;
            cmds.erase(c2);
            cmds.erase(c3);
            return true;
        }
    }
    return false;
}
bool o_const_expression2(VMCommandList& cmds, VMCommandList::iterator& c1,
                         VMCommandList::iterator& c2)
{
    if (c1->type == VMCommand::CONSTANT && c2->type == VMCommand::UNARY) {
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
bool o_const_if(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2)
{
    if (c1->type != VMCommand::CONSTANT || c2->type != VMCommand::IF) {
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
        c2->type = VMCommand::GOTO;
    } else {
        cmds.erase(c2);
    }
    cmds.erase(c1);
    return true;
}
/*
 * convert binary operation to unary
 */
bool o_const_swap(VMCommandList&, VMCommandList::iterator& c1, VMCommandList::iterator& c2,
                  VMCommandList::iterator& c3)
{
    if (c1->type == VMCommand::CONSTANT && c2->type == VMCommand::PUSH) {
        if (c3->type == VMCommand::BINARY) {
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
        if (c3->type == VMCommand::COMPARE) {
            c3->compare.swap();
        }
        if (c3->type == VMCommand::BINARY || c3->type == VMCommand::COMPARE) {
            c1->segment1 = c2->segment1;
            c1->type = VMCommand::PUSH;
            c2->type = VMCommand::CONSTANT;
            int temp = c1->int1;
            c1->int1 = c2->int1;
            c2->int1 = temp;
            return true;
        }
    }
    return false;
}
bool o_binary_to_unary(VMCommandList& cmds, VMCommandList::iterator& c1,
                       VMCommandList::iterator& c2)
{
    if (c1->type == VMCommand::CONSTANT) {
        if (c2->type == VMCommand::BINARY) {
            c2->type = VMCommand::UNARY;
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
        if (c2->type == VMCommand::COMPARE) {
            c2->type = VMCommand::UNARY_COMPARE;
            c2->int1 = c1->int1;
            cmds.erase(c1);
            return true;
        }
    }
    return false;
}
void o_special_unary(VMCommandList& cmds, VMCommandList::iterator& c1)
{
    if (c1->type != VMCommand::UNARY)
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
bool o_binary_equalarg(VMCommandList& cmds, VMCommandList::iterator& c1,
                       VMCommandList::iterator& c2, VMCommandList::iterator& c3)
{
    if (c1->type == VMCommand::PUSH && c2->type == VMCommand::PUSH && c1->segment1 == c2->segment1
        && c1->int1 == c2->int1) {
        if (c3->type == VMCommand::BINARY && c3->binary == ADD) {
            c3->type = VMCommand::UNARY;
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
bool o_push_pop(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2)
{
    if (c1->type != VMCommand::PUSH || c2->type != VMCommand::POP_INDIRECT) {
        return false;
    }

    c1->type = VMCommand::COPY;
    c1->int2 = c2->int1;
    c1->segment2 = c2->segment1;
    cmds.erase(c2);
    return true;
}
bool o_compare_if(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2)
{
    if (c2->type == VMCommand::IF) {
        if (c1->type == VMCommand::COMPARE) {
            c1->type = VMCommand::COMPARE_IF;
            c1->arg1 = c2->arg1;
            cmds.erase(c2);
            return true;
        }
        if (c1->type == VMCommand::UNARY_COMPARE) {
            c1->type = VMCommand::UNARY_COMPARE_IF;
            c1->arg1 = c2->arg1;
            cmds.erase(c2);
            return true;
        }
    }
    return false;
}
bool o_goto_goto(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2)
{
    if (c1->type != VMCommand::GOTO || c2->type != VMCommand::GOTO) {
        return false;
    }

    cmds.erase(c2);
    return true;
}
bool o_pop_push(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2)
{
    if (c1->type == VMCommand::POP_INDIRECT && c2->type == VMCommand::PUSH
        && c1->segment1 == c2->segment1 && c1->int1 == c2->int1) {
        c1->type = VMCommand::POP_INDIRECT_PUSH;
        cmds.erase(c2);
        return true;
    }
    return false;
}
/*
 * stack-less computation chains
 */
void s_replicate(VMCommandList& cmds, VMCommandList::iterator& c1)
{
    VMCommand in;
    in.type = VMCommand::IN;

    switch (c1->type) {
    case VMCommand::UNARY:
    case VMCommand::BINARY:
    case VMCommand::COMPARE:
    case VMCommand::UNARY_COMPARE:
    case VMCommand::IF:
    case VMCommand::COMPARE_IF:
    case VMCommand::UNARY_COMPARE_IF:
    case VMCommand::POP_DIRECT:
        c1->in = false;
        cmds.insert(c1, in);
        break;
    case VMCommand::PUSH:
    case VMCommand::POP_INDIRECT_PUSH:
    case VMCommand::CONSTANT:
    default:
        break;
    }
    switch (c1->type) {
    case VMCommand::UNARY:
    case VMCommand::BINARY:
    case VMCommand::COMPARE:
    case VMCommand::UNARY_COMPARE:
    case VMCommand::PUSH:
    case VMCommand::CONSTANT:
    case VMCommand::POP_INDIRECT_PUSH:
        c1->fin = false;
        cmds.insert(c1, *c1);
        c1->type = VMCommand::FIN;
        break;
    case VMCommand::IF:
    case VMCommand::COMPARE_IF:
    case VMCommand::UNARY_COMPARE_IF:
    case VMCommand::POP_DIRECT:
    default:
        break;
    }
}
bool s_reduce(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2)
{
    if (c1->type != VMCommand::FIN || c2->type != VMCommand::IN) {
        return false;
    }

    cmds.erase(c1);
    cmds.erase(c2);
    return true;
}
bool s_reconstruct(VMCommandList& cmds, VMCommandList::iterator& c1, VMCommandList::iterator& c2)
{
    if (c1->type == VMCommand::IN) {
        c2->in = true;
        cmds.erase(c1);
        return true;
    }
    if (c2->type == VMCommand::FIN) {
        c1->fin = true;
        cmds.erase(c2);
        return true;
    }
    return false;
}

void VMOptimize(VMCommandList& cmds)
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

} // namespace hcc {
