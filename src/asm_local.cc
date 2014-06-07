// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#include "asm.h"
#include <algorithm>
#include <cassert>

namespace {

using namespace hcc::instruction;

const hcc::asm_instruction NOP = {
    hcc::asm_instruction_type::VERBATIM,
    "",
    COMPUTE | RESERVED | COMP_ZERO,
};

//=============================================================================
// LOCAL CONSTANT PROPAGATION
//=============================================================================

//-----------------------------------------------------------------------------
// Represents knowledge about value in registers
//-----------------------------------------------------------------------------
enum class constant_value_type {
    UNKNOWN,
    ZERO,
    NEGATIVE_ONE,
    POSITIVE_ONE,
    OTHER,
};
struct constant_value {
    constant_value(
        constant_value_type type = constant_value_type::UNKNOWN,
        std::string other = "")
            : type(std::move(type))
            , other(std::move(other))
    { }

    constant_value_type type;
    std::string other;
};
bool operator==(const constant_value& a, const constant_value& b)
{
    if (a.type != b.type)
        return false;
    if (a.type == constant_value_type::OTHER && a.other != b.other)
        return false;
    return true;
}
struct registers {
    constant_value D;
    constant_value A;
    constant_value M;
};

//-----------------------------------------------------------------------------
// Propagate constants in "comp" part of C-instruction
//-----------------------------------------------------------------------------
// Despite ugliness, this is just a simple mapping
//
//      (comp x D x A x M) -> comp
//
// This needs to be called twice to be sure it converged, as each iteration
// lowers arity by one.
//-----------------------------------------------------------------------------
uint16_t simplify(uint16_t comp, const registers& r)
{
    switch (comp) {
    // nullary
    case COMP_ZERO:
    case COMP_ONE:
    case COMP_MINUS_ONE:
        return comp;
    // unary
    case COMP_D:
        switch (r.D.type) {
        case constant_value_type::ZERO:             return COMP_ZERO;
        case constant_value_type::POSITIVE_ONE:     return COMP_ONE;
        case constant_value_type::NEGATIVE_ONE:     return COMP_MINUS_ONE;
        default:                                    return comp;
        }
    case COMP_A:
        switch (r.A.type) {
        case constant_value_type::ZERO:             return COMP_ZERO;
        case constant_value_type::POSITIVE_ONE:     return COMP_ONE;
        case constant_value_type::NEGATIVE_ONE:     return COMP_MINUS_ONE;
        default:                                    return comp;
        }
    case COMP_M:
        switch (r.M.type) {
        case constant_value_type::ZERO:             return COMP_ZERO;
        case constant_value_type::POSITIVE_ONE:     return COMP_ONE;
        case constant_value_type::NEGATIVE_ONE:     return COMP_MINUS_ONE;
        default:                                    return comp;
        }
    case COMP_NOT_D:
        switch (r.D.type) {
        case constant_value_type::ZERO:             return COMP_MINUS_ONE;
        case constant_value_type::NEGATIVE_ONE:     return COMP_ZERO;
        default:                                    return comp;
        }
    case COMP_NOT_A:
        switch (r.A.type) {
        case constant_value_type::ZERO:             return COMP_MINUS_ONE;
        case constant_value_type::NEGATIVE_ONE:     return COMP_ZERO;
        default:                                    return comp;
        }
    case COMP_NOT_M:
        switch (r.M.type) {
        case constant_value_type::ZERO:             return COMP_MINUS_ONE;
        case constant_value_type::NEGATIVE_ONE:     return COMP_ZERO;
        default:                                    return comp;
        }
    case COMP_MINUS_D:
        switch (r.D.type) {
        case constant_value_type::ZERO:             return COMP_ZERO;
        case constant_value_type::NEGATIVE_ONE:     return COMP_ONE;
        case constant_value_type::POSITIVE_ONE:     return COMP_MINUS_ONE;
        default:                                    return comp;
        }
    case COMP_MINUS_A:
        switch (r.A.type) {
        case constant_value_type::ZERO:             return COMP_ZERO;
        case constant_value_type::NEGATIVE_ONE:     return COMP_ONE;
        case constant_value_type::POSITIVE_ONE:     return COMP_MINUS_ONE;
        default:                                    return comp;
        }
    case COMP_MINUS_M:
        switch (r.M.type) {
        case constant_value_type::ZERO:             return COMP_ZERO;
        case constant_value_type::NEGATIVE_ONE:     return COMP_ONE;
        case constant_value_type::POSITIVE_ONE:     return COMP_MINUS_ONE;
        default:                                    return comp;
        }
    case COMP_D_PLUS_ONE:
        switch (r.D.type) {
        case constant_value_type::ZERO:             return COMP_ONE;
        case constant_value_type::NEGATIVE_ONE:     return COMP_ZERO;
        default:                                    return comp;
        }
    case COMP_A_PLUS_ONE:
        switch (r.A.type) {
        case constant_value_type::ZERO:             return COMP_ONE;
        case constant_value_type::NEGATIVE_ONE:     return COMP_ZERO;
        default:                                    return comp;
        }
    case COMP_M_PLUS_ONE:
        switch (r.M.type) {
        case constant_value_type::ZERO:             return COMP_ONE;
        case constant_value_type::NEGATIVE_ONE:     return COMP_ZERO;
        default:                                    return comp;
        }
    case COMP_D_MINUS_ONE:
        switch (r.D.type) {
        case constant_value_type::ZERO:             return COMP_MINUS_ONE;
        case constant_value_type::POSITIVE_ONE:     return COMP_ZERO;
        default:                                    return comp;
        }
    case COMP_A_MINUS_ONE:
        switch (r.A.type) {
        case constant_value_type::ZERO:             return COMP_MINUS_ONE;
        case constant_value_type::POSITIVE_ONE:     return COMP_ZERO;
        default:                                    return comp;
        }
    case COMP_M_MINUS_ONE:
        switch (r.M.type) {
        case constant_value_type::ZERO:             return COMP_MINUS_ONE;
        case constant_value_type::POSITIVE_ONE:     return COMP_ZERO;
        default:                                    return comp;
        }
    case COMP_D_PLUS_A:
        switch (r.D.type) {
        case constant_value_type::POSITIVE_ONE:     return COMP_A_PLUS_ONE;
        case constant_value_type::ZERO:             return COMP_A;
        case constant_value_type::NEGATIVE_ONE:     return COMP_A_MINUS_ONE;
        default:
            switch (r.A.type) {
            case constant_value_type::POSITIVE_ONE: return COMP_D_PLUS_ONE;
            case constant_value_type::ZERO:         return COMP_D;
            case constant_value_type::NEGATIVE_ONE: return COMP_D_MINUS_ONE;
            default:                                return comp;
            }
        }
    // binary
    case COMP_D_MINUS_A:
        switch (r.A.type) {
        case constant_value_type::POSITIVE_ONE:     return COMP_D_MINUS_ONE;
        case constant_value_type::ZERO:             return COMP_D;
        case constant_value_type::NEGATIVE_ONE:     return COMP_D_PLUS_ONE;
        default:
            switch (r.D.type) {
            case constant_value_type::ZERO:         return COMP_MINUS_A;
            default:                                return comp;
            }
        }
    case COMP_A_MINUS_D:
        switch (r.D.type) {
        case constant_value_type::POSITIVE_ONE:     return COMP_A_MINUS_ONE;
        case constant_value_type::ZERO:             return COMP_A;
        case constant_value_type::NEGATIVE_ONE:     return COMP_A_PLUS_ONE;
        default:
            switch (r.A.type) {
            case constant_value_type::ZERO:         return COMP_MINUS_D;
            default:                                return comp;
            }
        }
    case COMP_D_AND_A:
        switch (r.D.type) {
        case constant_value_type::ZERO:             return COMP_ZERO;
        case constant_value_type::NEGATIVE_ONE:     return COMP_A;
        default:
            switch (r.A.type) {
            case constant_value_type::ZERO:         return COMP_ZERO;
            case constant_value_type::NEGATIVE_ONE: return COMP_D;
            default:                                return comp;
            }
        }
    case COMP_D_OR_A:
        switch (r.D.type) {
        case constant_value_type::ZERO:             return COMP_A;
        case constant_value_type::NEGATIVE_ONE:     return COMP_MINUS_ONE;
        default:
            switch (r.A.type) {
            case constant_value_type::ZERO:         return COMP_D;
            case constant_value_type::NEGATIVE_ONE: return COMP_MINUS_ONE;
            default:                                return comp;
            }
        }
    case COMP_D_PLUS_M:
        switch (r.D.type) {
        case constant_value_type::POSITIVE_ONE:     return COMP_M_PLUS_ONE;
        case constant_value_type::ZERO:             return COMP_M;
        case constant_value_type::NEGATIVE_ONE:     return COMP_M_MINUS_ONE;
        default:
            switch (r.M.type) {
            case constant_value_type::POSITIVE_ONE: return COMP_D_PLUS_ONE;
            case constant_value_type::ZERO:         return COMP_D;
            case constant_value_type::NEGATIVE_ONE: return COMP_D_MINUS_ONE;
            default:                                return comp;
            }
        }
    case COMP_D_MINUS_M:
        switch (r.M.type) {
        case constant_value_type::POSITIVE_ONE:     return COMP_D_MINUS_ONE;
        case constant_value_type::ZERO:             return COMP_D;
        case constant_value_type::NEGATIVE_ONE:     return COMP_D_PLUS_ONE;
        default:
            switch (r.D.type) {
            case constant_value_type::ZERO:         return COMP_MINUS_M;
            default:                                return comp;
            }
        }
    case COMP_M_MINUS_D:
        switch (r.D.type) {
        case constant_value_type::POSITIVE_ONE:     return COMP_M_MINUS_ONE;
        case constant_value_type::ZERO:             return COMP_M;
        case constant_value_type::NEGATIVE_ONE:     return COMP_M_PLUS_ONE;
        default:
            switch (r.M.type) {
            case constant_value_type::ZERO:         return COMP_MINUS_D;
            default:                                return comp;
            }
        }
    case COMP_D_AND_M:
        switch (r.D.type) {
        case constant_value_type::ZERO:             return COMP_ZERO;
        case constant_value_type::NEGATIVE_ONE:     return COMP_M;
        default:
            switch (r.M.type) {
            case constant_value_type::ZERO:         return COMP_ZERO;
            case constant_value_type::NEGATIVE_ONE: return COMP_D;
            default:                                return comp;
            }
        }
    case COMP_D_OR_M:
        switch (r.D.type) {
        case constant_value_type::ZERO:             return COMP_M;
        case constant_value_type::NEGATIVE_ONE:     return COMP_MINUS_ONE;
        default:
            switch (r.M.type) {
            case constant_value_type::ZERO:         return COMP_D;
            case constant_value_type::NEGATIVE_ONE: return COMP_MINUS_ONE;
            default:                                return comp;
            }
        }
    default:
        assert(false && "Undocumented instruction");
    }
}

//-----------------------------------------------------------------------------
// Propagate constants in asm program
//-----------------------------------------------------------------------------
struct constant_propagation {
    void operator()(hcc::asm_instruction& cmd)
    {
        auto handle_ainstr = [&] (const constant_value& new_A) {
            if (r.A == new_A) {
                cmd = NOP;
            } else {
                r.A = new_A;
                r.M.type = constant_value_type::UNKNOWN;
            }
        };

        switch (cmd.type) {
        case hcc::asm_instruction_type::LABEL:
            // reset
            r = registers();
            break;
        case hcc::asm_instruction_type::COMMENT:
            // nothing to do here
            break;
        case hcc::asm_instruction_type::LOAD:
            handle_ainstr(constant_value(
                constant_value_type::OTHER,
                "@" + cmd.symbol));
            break;
        case hcc::asm_instruction_type::VERBATIM:
            if (cmd.instr & COMPUTE) {
                // C-instruction

                // try to simplify
                uint16_t comp = cmd.instr & MASK_COMP;
                comp = simplify(comp, r);
                comp = simplify(comp, r);
                cmd.instr = (cmd.instr & ~MASK_COMP) | comp;

                constant_value result;
                switch (comp) {
                case COMP_ZERO:
                    result.type = constant_value_type::ZERO;
                    break;
                case COMP_ONE:
                    result.type = constant_value_type::POSITIVE_ONE;
                    break;
                case COMP_MINUS_ONE:
                    result.type = constant_value_type::NEGATIVE_ONE;
                    break;
                case COMP_D:
                    cmd.instr &= ~DEST_D;
                    result = r.D;
                    break;
                case COMP_A:
                    result = r.A;
                    break;
                case COMP_M:
                    result = r.M;
                    break;
                default:
                    // nothing
                    break;
                }

                if (cmd.instr & DEST_D) {
                    r.D = result;
                }
                if (cmd.instr & DEST_A) {
                    r.A = result;
                }
                if (cmd.instr & DEST_M) {
                    r.M = result;
                }

                if ((cmd.instr & (MASK_DEST | MASK_JUMP)) == 0) {
                    cmd = NOP;
                }
            } else {
                // A-instruction

                if (cmd.instr == 0) {
                    handle_ainstr(constant_value(
                        constant_value_type::ZERO));
                } else if (cmd.instr == 1) {
                    handle_ainstr(constant_value(
                        constant_value_type::POSITIVE_ONE));
                } else {
                    handle_ainstr(constant_value(
                        constant_value_type::OTHER,
                        std::to_string(cmd.instr)));
                }
            }
        }
    }

private:
    registers r;
};

//=============================================================================
// LOCAL DEAD CODE ELIMINATION
//=============================================================================
struct dead_code_elimination {
    void operator()(hcc::asm_instruction& command)
    {
        bool live = false;
        bool provide_A = false;
        bool provide_D = false;
        switch (command.type) {
        case hcc::asm_instruction_type::VERBATIM:
            if (command.instr & COMPUTE) {
                if ((command.instr & DEST_M) || (command.instr & MASK_JUMP)) {
                    live = true;
                }
                if (command.instr & DEST_A) {
                    provide_A = true;
                }
                if (command.instr & DEST_D) {
                    provide_D = true;
                }
            } else {
                provide_A = true;
            }
            break;
        case hcc::asm_instruction_type::LABEL:
        case hcc::asm_instruction_type::COMMENT:
            live = true;
            break;
        case hcc::asm_instruction_type::LOAD:
            provide_A = true;
            break;
        }

        if (require_A && provide_A) {
            live = true;
            require_A = false;
        }
        if (require_D && provide_D) {
            live = true;
            require_D = false;
        }

        if (live) {
            if (command.type == hcc::asm_instruction_type::VERBATIM) {
                if (command.instr & COMPUTE) {
                    if ((command.instr & MASK_JUMP) ||
                        (command.instr & DEST_M))
                    {
                        require_A = true;
                    }
                    switch (command.instr & MASK_COMP) {
                    case COMP_ZERO:
                    case COMP_ONE:
                    case COMP_MINUS_ONE:
                        break;
                    case COMP_D:
                    case COMP_NOT_D:
                    case COMP_MINUS_D:
                    case COMP_D_PLUS_ONE:
                    case COMP_D_MINUS_ONE:
                        require_D = true;
                        break;
                    case COMP_A:
                    case COMP_NOT_A:
                    case COMP_MINUS_A:
                    case COMP_A_PLUS_ONE:
                    case COMP_A_MINUS_ONE:
                    case COMP_M:
                    case COMP_NOT_M:
                    case COMP_MINUS_M:
                    case COMP_M_PLUS_ONE:
                    case COMP_M_MINUS_ONE:
                        require_A = true;
                        break;
                    case COMP_D_PLUS_A:
                    case COMP_D_MINUS_A:
                    case COMP_A_MINUS_D:
                    case COMP_D_AND_A:
                    case COMP_D_OR_A:
                    case COMP_D_PLUS_M:
                    case COMP_D_MINUS_M:
                    case COMP_M_MINUS_D:
                    case COMP_D_AND_M:
                    case COMP_D_OR_M:
                        require_D = true;
                        require_A = true;
                        break;
                    default:
                        assert(false && "Undocumented instruction");
                    }
                }
            }
        } else {
            command = NOP;
        }
    }

private:
    bool require_A = false;
    bool require_D = false;
};

bool is_nop(const hcc::asm_instruction& i)
{
    return i == NOP;
}

} // anonymous namespace

namespace hcc {

//=============================================================================
// DRIVER
//=============================================================================
void asm_program::local_optimization()
{
    // two iterations are usually enough
    for (int i = 0; i<2; ++i) {
        std::for_each(instructions.begin(), instructions.end(),
            constant_propagation());

        std::for_each(instructions.rbegin(), instructions.rend(),
            dead_code_elimination());

        instructions.erase(
            std::remove_if(instructions.begin(), instructions.end(), is_nop),
            instructions.end());
    }
}

} // namespace hcc
