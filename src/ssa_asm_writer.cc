// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "asm.h"
#include "instruction.h"
#include "ssa.h"
#include <sstream>

namespace hcc {
namespace ssa {

namespace {

using namespace hcc::instruction;

// registers 11-15 are reserved for implementation
const std::string reg_locals        = "R11";
const std::string reg_arguments     = "R12";
const std::string reg_stack_pointer = "R13";
const std::string reg_tmp           = "R14";
const std::string reg_return        = "R15";
const std::vector<std::string> saved_registers = {
    "R0",
    "R1",
    "R2",
    "R3",
    "R4",
    "R5",
    "R6",
    reg_arguments,
    reg_locals,
};

// Symmetric binary operations yield the same result despite argument order.
// Local asssembler optimizations sometimes perform better if we swap arguments.
void adjust_symmetric_operation(instruction &i)
{
    if (i.arguments[1] == i.arguments[0]) {
        std::swap(i.arguments[1], i.arguments[2]);
    }
}

struct subroutine_writer {
    subroutine_writer(unit& u, asm_program& out, const global& prefix, subroutine& s)
        : u(u)
        , out(out)
        , s(s)
        , prefix(u.globals.get(prefix))
    {
        int available_register = 0;

        auto f = [&] (const argument& arg) {
            if (arg.is_reg()) {
                const auto& x = arg.get_reg();
                if (registers.count(x) == 0) {
                    registers[x] = "R" + std::to_string(available_register++);
                }
            } else if (arg.is_local()) {
                const auto& x = arg.get_local();
                auto it = locals_counts.find(x);
                if (it == locals_counts.end()) {
                    locals_counts.emplace(x, 1);
                } else {
                    it->second++;
                }
            }
        };

        s.for_each_bb([&] (basic_block& bb) {
            for (auto& instruction : bb.instructions) {
                instruction.def_apply(f);
                instruction.use_apply(f);
                if (instruction.type == instruction_type::STORE) {
                    f(instruction.arguments[0]);
                }
                if (instruction.type == instruction_type::LOAD) {
                    f(instruction.arguments[1]);
                }
            }
        });

        std::vector<std::pair<local, int>> locals_tmp;
        for (const auto& kv : locals_counts) {
            locals_tmp.emplace_back(kv.first, kv.second);
        }
        std::sort(locals_tmp.begin(), locals_tmp.end(), [] (const std::pair<local, int>& x, const std::pair<local, int>& y) { return x.second > y.second; });
        locals_counts.clear();
        for (const auto& kv : locals_tmp) {
            locals_counts.emplace(kv.first, locals_counter++);
        }
    }

    void write_subroutine()
    {
        s.for_each_bb([&] (basic_block& bb) {
            write_basic_block(bb);
        });
    }

private:
    void write_basic_block(basic_block& bb)
    {
        if (s.exit_node().name == bb.name) {
            return;
        }
        if (s.entry_node().name == bb.name) {
            out.emitL(prefix);
        }
        out.emitL(handle_label(bb.name));
        for (auto instruction : bb.instructions) {
            write_instruction(instruction);
        }
    }

    void write_instruction(hcc::ssa::instruction& instruction)
    {
        out.emitComment(instruction.save_fast());

        switch (instruction.type) {
        // basic block boundary handling
        case instruction_type::JUMP:
            out.emitA(handle_label(instruction.arguments[0].get_label()));
            out.emitC(COMP_ZERO | JMP);
            break;
        case instruction_type::JLT:
            write_compare(instruction, JLT);
            break;
        case instruction_type::JEQ:
            write_compare(instruction, JEQ);
            break;
        // subroutine boundary handling
        case instruction_type::RETURN:
            handle(instruction.arguments[0], COMP_M, COMP_A);
            out.emitA(reg_return);
            out.emitC(DEST_M | COMP_D);
            out.emitA("__returnHelper");
            out.emitC(COMP_ZERO | JMP);
            break;
        case instruction_type::CALL:
            write_call(instruction);
            break;
        // arithmetic instructions
        case instruction_type::ADD:
            adjust_symmetric_operation(instruction);
            handle(instruction.arguments[1], COMP_M, COMP_A);
            handle(instruction.arguments[2], COMP_D_PLUS_M, COMP_D_PLUS_A);
            reg_store(instruction.arguments[0]);
            break;
        case instruction_type::SUB:
            handle(instruction.arguments[1], COMP_M, COMP_A);
            handle(instruction.arguments[2], COMP_D_MINUS_M, COMP_D_MINUS_A);
            reg_store(instruction.arguments[0]);
            break;
        case instruction_type::AND:
            adjust_symmetric_operation(instruction);
            handle(instruction.arguments[1], COMP_M, COMP_A);
            handle(instruction.arguments[2], COMP_D_AND_M, COMP_D_AND_A);
            reg_store(instruction.arguments[0]);
            break;
        case instruction_type::OR:
            adjust_symmetric_operation(instruction);
            handle(instruction.arguments[1], COMP_M, COMP_A);
            handle(instruction.arguments[2], COMP_D_OR_M, COMP_D_OR_A);
            reg_store(instruction.arguments[0]);
            break;
        case instruction_type::NOT:
            handle(instruction.arguments[1], COMP_NOT_M, COMP_NOT_A);
            reg_store(instruction.arguments[0]);
            break;
        case instruction_type::NEG:
            handle(instruction.arguments[1], COMP_MINUS_M, COMP_MINUS_A);
            reg_store(instruction.arguments[0]);
            break;
        // copying
        case instruction_type::STORE:
            write_store(instruction.arguments[0], instruction.arguments[1]);
            break;
        case instruction_type::ARGUMENT:
            write_argument(instruction.arguments[0], instruction.arguments[1]);
            break;
        case instruction_type::LOAD:
            write_load(instruction.arguments[0], instruction.arguments[1]);
            break;
        case instruction_type::MOV:
            handle(instruction.arguments[1], COMP_M, COMP_A);
            reg_store(instruction.arguments[0]);
            break;
        default:
            assert(false);
        }
    }

    void write_compare(const hcc::ssa::instruction& instruction, unsigned short jump)
    {
        // compute
        handle(instruction.arguments[1], COMP_M, COMP_A);
        handle(instruction.arguments[0], COMP_M_MINUS_D, COMP_A_MINUS_D);
        // decide
        out.emitA(handle_label(instruction.arguments[2].get_label()));
        out.emitC(COMP_D | jump);
        out.emitA(handle_label(instruction.arguments[3].get_label()));
        out.emitC(COMP_ZERO | JMP);
    }

    void write_call(const hcc::ssa::instruction& instruction)
    {
        const auto return_address = prefix + ".return." + std::to_string(return_counter++);
        out.emitA(locals_counter);
        out.emitC(DEST_D | COMP_A);
        out.emitA(reg_locals);
        out.emitC(DEST_D | COMP_D_PLUS_M);
        out.emitA(reg_stack_pointer);
        out.emitC(DEST_M | COMP_D_MINUS_ONE);
        out.emitA(reg_tmp);
        out.emitC(DEST_M | COMP_D);
        for (int i = 2; i < static_cast<int>(instruction.arguments.size()); ++i) {
            handle(instruction.arguments[i], COMP_M, COMP_A);
            out.emitA(reg_stack_pointer);
            out.emitC(DEST_A | DEST_M | COMP_M_PLUS_ONE);
            out.emitC(DEST_M | COMP_D);
        }
        out.emitA(return_address);
        out.emitC(DEST_D | COMP_A);
        out.emitA(reg_stack_pointer);
        out.emitC(DEST_A | DEST_M | COMP_M_PLUS_ONE);
        out.emitC(DEST_M | COMP_D);
        emit_global(instruction.arguments[1].get_global());
        out.emitC(DEST_D | COMP_A);
        out.emitA(reg_return);
        out.emitC(DEST_M | COMP_D);
        out.emitA("__callHelper");
        out.emitC(COMP_ZERO | JMP);
        out.emitL(return_address);
        out.emitA(reg_return);
        out.emitC(DEST_D | COMP_M);
        reg_store(instruction.arguments[0]);
    }

    void write_store(const argument& dst, const argument& src)
    {
        if (dst.is_reg()) {
            handle(src, COMP_M, COMP_A);
            emit_register(dst.get_reg());
            out.emitC(DEST_A | COMP_M);
            out.emitC(DEST_M | COMP_D);
        } else if (dst.is_global()) {
            handle(src, COMP_M, COMP_A);
            emit_global(dst.get_global());
            out.emitC(DEST_M | COMP_D);
        } else if (dst.is_local()) {
            out.emitA(locals_counts.at(dst.get_local()));
            out.emitC(DEST_D | COMP_A);
            out.emitA(reg_locals);
            out.emitC(DEST_D | COMP_D_PLUS_M);
            out.emitA(reg_tmp);
            out.emitC(DEST_M | COMP_D);
            handle(src, COMP_M, COMP_A);
            out.emitA(reg_tmp);
            out.emitC(DEST_A | COMP_M);
            out.emitC(DEST_M | COMP_D);
        } else {
            assert (false);
        }
    }

    void write_argument(const argument& dst, const argument& index)
    {
        out.emitA(index.get_constant().value);
        out.emitC(DEST_D | COMP_A);
        out.emitA(reg_arguments);
        out.emitC(DEST_A | COMP_D_PLUS_M);
        out.emitC(DEST_D | COMP_M);
        reg_store(dst);
    }

    void write_load(const argument& dst, const argument& src)
    {
        if (src.is_reg()) {
            emit_register(src.get_reg());
            out.emitC(DEST_A | COMP_M);
            out.emitC(DEST_D | COMP_M);
        } else if (src.is_global()) {
            emit_global(src.get_global());
            out.emitC(DEST_D | COMP_M);
        } else if (src.is_local()) {
            out.emitA(locals_counts.at(src.get_local()));
            out.emitC(DEST_D | COMP_A);
            out.emitA(reg_locals);
            out.emitC(DEST_A | COMP_D_PLUS_M);
            out.emitC(DEST_D | COMP_M);
        } else {
            assert (false);
        }
        reg_store(dst);
    }

    void emit_register(const reg& x) { out.emitA(registers.at(x)); }
    void emit_global(const global& x) { out.emitA(u.globals.get(x)); }

    void handle(const argument& arg, unsigned short comp_reg, unsigned short comp_imm)
    {
        if (arg.is_constant()) {
            out.emitA(arg.get_constant().value);
            out.emitC(DEST_D | comp_imm);
        } else if (arg.is_reg()) {
            emit_register(arg.get_reg());
            out.emitC(DEST_D | comp_reg);
        } else if (arg.is_global()) {
            emit_global(arg.get_global());
            out.emitC(DEST_D | comp_reg);
        } else {
            assert(false);
        }
    }

    std::string handle_label(const label& l)
    {
        return prefix + argument(l).save_fast();
    }

    void reg_store(const argument& x)
    {
        assert (x.is_reg());
        emit_register(x.get_reg());
        out.emitC(DEST_M | COMP_D);
    }

    unit& u;
    asm_program& out;
    subroutine& s;
    const std::string prefix;
    std::map<reg, std::string> registers;
    std::map<local, int> locals_counts;
    int return_counter = 0;
    int locals_counter = 0;
};

struct asm_writer {
    asm_writer(asm_program& out) : out(out) { }

    void write(unit& u)
    {
        write_bootstrap();
        write_return_helper();
        write_call_helper();
        for (auto& s: u.subroutines) {
            subroutine_writer sw {u, out, s.first, s.second};
            sw.write_subroutine();
        }
    }

private:
    void write_bootstrap()
    {
        const int reserved = 256;

        // call Sys.init and halt
        // store return address
        out.emitA("__halt");
        out.emitC(DEST_D | COMP_A);
        out.emitA(reserved);
        out.emitC(DEST_M | COMP_D);
        // set reg_localc for called subroutine
        out.emitA(reserved + saved_registers.size() + 1);
        out.emitC(DEST_D | COMP_A);
        out.emitA(reg_locals);
        out.emitC(DEST_M | COMP_D);
        // jump
        out.emitA("Sys.init");
        out.emitL("__halt");
        out.emitC(COMP_ZERO | JMP);
    }

    void write_return_helper()
    {
        out.emitL("__returnHelper");
        out.emitA(reg_locals);
        out.emitC(DEST_D | COMP_M);
        out.emitA(reg_stack_pointer);
        out.emitC(DEST_M | COMP_D);
        for (auto i = saved_registers.begin(), e = saved_registers.end(); i != e; ++i) {
            // pop
            out.emitA(reg_stack_pointer);
            out.emitC(DEST_A | DEST_M | COMP_M_MINUS_ONE);
            out.emitC(DEST_D | COMP_M);
            out.emitA(*i);
            out.emitC(DEST_M | COMP_D);
        }
        out.emitA(reg_stack_pointer);
        out.emitC(DEST_A | DEST_M | COMP_M_MINUS_ONE);
        out.emitC(DEST_A | COMP_M);
        out.emitC(COMP_ZERO | JMP);
    }

    void write_call_helper()
    {
        out.emitL("__callHelper");
        for (auto i = saved_registers.rbegin(), e = saved_registers.rend(); i != e; ++i) {
            out.emitA(*i);
            out.emitC(DEST_D | COMP_M);
            out.emitA(reg_stack_pointer);
            out.emitC(DEST_A | DEST_M | COMP_M_PLUS_ONE);
            out.emitC(DEST_M | COMP_D);
        }
        // set reg_locals for called subroutine
        out.emitA(reg_stack_pointer);
        out.emitC(DEST_D | COMP_M_PLUS_ONE);
        out.emitA(reg_locals);
        out.emitC(DEST_M | COMP_D);
        // set reg_arguments for called subroutine
        out.emitA(reg_tmp);
        out.emitC(DEST_D | COMP_M);
        out.emitA(reg_arguments);
        out.emitC(DEST_M | COMP_D);
        // jump to reg_return
        out.emitA(reg_return);
        out.emitC(DEST_A | COMP_M);
        out.emitC(COMP_ZERO | JMP);
    }

    asm_program& out;
};

} // anonymous namespace

void unit::translate_to_asm(asm_program& out)
{
    asm_writer w {out};
    w.write(*this);
}

} // namespace ssa
} // namespace hcc
