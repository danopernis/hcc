// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "asm.h"
#include "instruction.h"
#include "ssa.h"
#include <boost/algorithm/string/join.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace hcc::ssa;
using namespace hcc::instruction;

// registers 11-15 are reserved for implementation
const std::string reg_locals        = "R11";
const std::string reg_arguments     = "R12";
const std::string reg_stack_pointer = "R13";
const std::string reg_tmp           = "R14";
const std::string reg_return        = "R15";

inline void push(hcc::asm_program& out, const std::string& reg)
{
    out.emitA(reg);
    out.emitC(DEST_D | COMP_M);
    out.emitA(reg_stack_pointer);
    out.emitC(DEST_A | DEST_M | COMP_M_PLUS_ONE);
    out.emitC(DEST_M | COMP_D);
}

// Symmetric binary operations yield the same result despite argument order.
// Local asssembler optimizations sometimes perform better if we swap arguments.
void adjust_symmetric_operation(instruction &i)
{
    if (i.arguments[1] == i.arguments[0]) {
        std::swap(i.arguments[1], i.arguments[2]);
    }
}

void generate_code(unit& u, subroutine& s, hcc::asm_program& out, const global& prefix)
{
    int available_register = 0;
    std::map<reg, std::string> registers;
    std::map<local, int> locals_counts;

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
    int locals_counter = 0;
    locals_counts.clear();
    for (const auto& kv : locals_tmp) {
        locals_counts.emplace(kv.first, locals_counter++);
    }

    int returnCounter = 0;
    auto handle = [&] (const argument& arg, unsigned short comp_reg, unsigned short comp_imm) {
        if (arg.is_constant()) {
            out.emitA(arg.get_constant().value);
            out.emitC(DEST_D | comp_imm);
        } else if (arg.is_reg()) {
            out.emitA(registers.at(arg.get_reg()));
            out.emitC(DEST_D | comp_reg);
        } else if (arg.is_global()) {
            out.emitA(u.globals.get(arg.get_global()));
            out.emitC(DEST_D | comp_reg);
        } else {
            assert(false);
        }
    };
    auto handleLabel = [&] (const label& l) {
        std::stringstream ss;
        ss << u.globals.get(prefix) << ".";
        argument(l).save(ss, u, s);
        return ss.str();
    };

    auto handle_compare = [&] (const hcc::ssa::instruction& instruction, unsigned short jump) {
        // compute
        handle(instruction.arguments[1], COMP_M, COMP_A);
        handle(instruction.arguments[0], COMP_M_MINUS_D, COMP_A_MINUS_D);
        // decide
        out.emitA(handleLabel(instruction.arguments[2].get_label()));
        out.emitC(COMP_D | jump);
        out.emitA(handleLabel(instruction.arguments[3].get_label()));
        out.emitC(COMP_ZERO | JMP);
    };
    auto reg_store = [&] (const argument& x) {
        assert (x.is_reg());
        out.emitA(registers.at(x.get_reg()));
        out.emitC(DEST_M | COMP_D);
    };

    s.for_each_bb([&] (basic_block& bb) {
        if (s.exit_node().name == bb.name) {
            return;
        }
        if (s.entry_node().name == bb.name) {
            out.emitL(u.globals.get(prefix));
        }
        out.emitL(handleLabel(bb.name));
    for (auto instruction : bb.instructions) {
        {
            std::stringstream ss;
            instruction.save(ss, u, s);
            out.emitComment(ss.str());
        }
        switch (instruction.type) {
        // basic block boundary handling
        case instruction_type::JUMP:
            out.emitA(handleLabel(instruction.arguments[0].get_label()));
            out.emitC(COMP_ZERO | JMP);
            break;
        case instruction_type::JLT:
            handle_compare(instruction, JLT);
            break;
        case instruction_type::JEQ:
            handle_compare(instruction, JEQ);
            break;
        // subroutine boundary handling
        case instruction_type::RETURN:
            handle(instruction.arguments[0], COMP_M, COMP_A);
            out.emitA(reg_return);
            out.emitC(DEST_M | COMP_D);
            out.emitA("__returnHelper");
            out.emitC(COMP_ZERO | JMP);
            break;
        case instruction_type::CALL: {
            const auto returnAddress = u.globals.get(prefix) + ".return." + std::to_string(returnCounter++);
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
            out.emitA(returnAddress);
            out.emitC(DEST_D | COMP_A);
            out.emitA(reg_stack_pointer);
            out.emitC(DEST_A | DEST_M | COMP_M_PLUS_ONE);
            out.emitC(DEST_M | COMP_D);
            out.emitA(u.globals.get(instruction.arguments[1].get_global()));
            out.emitC(DEST_D | COMP_A);
            out.emitA(reg_return);
            out.emitC(DEST_M | COMP_D);
            out.emitA("__callHelper");
            out.emitC(COMP_ZERO | JMP);
            out.emitL(returnAddress);
            out.emitA(reg_return);
            out.emitC(DEST_D | COMP_M);
            reg_store(instruction.arguments[0]);
            } break;
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
        case instruction_type::STORE: {
            const auto& dst = instruction.arguments[0];
            const auto& src = instruction.arguments[1];
            if (dst.is_reg()) {
                handle(src, COMP_M, COMP_A);
                out.emitA(registers.at(dst.get_reg()));
                out.emitC(DEST_A | COMP_M);
                out.emitC(DEST_M | COMP_D);
            } else if (dst.is_global()) {
                handle(src, COMP_M, COMP_A);
                out.emitA(u.globals.get(dst.get_global()));
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
            } break;
        case instruction_type::ARGUMENT:
            out.emitA(instruction.arguments[1].get_constant().value);
            out.emitC(DEST_D | COMP_A);
            out.emitA(reg_arguments);
            out.emitC(DEST_A | COMP_D_PLUS_M);
            out.emitC(DEST_D | COMP_M);
            reg_store(instruction.arguments[0]);
            break;
        case instruction_type::LOAD: {
            const auto& dst = instruction.arguments[0];
            const auto& src = instruction.arguments[1];
            if (src.is_reg()) {
                out.emitA(registers.at(src.get_reg()));
                out.emitC(DEST_A | COMP_M);
                out.emitC(DEST_D | COMP_M);
            } else if (src.is_global()) {
                out.emitA(u.globals.get(src.get_global()));
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
        } break;
        case instruction_type::MOV:
            handle(instruction.arguments[1], COMP_M, COMP_A);
            reg_store(instruction.arguments[0]);
            break;
        default:
            assert(false);
        }
    }
    });
}


int main(int argc, char* argv[])
try {
    if (argc < 2)
        throw std::runtime_error("Usage: ssa2asm input...\n");

    unit u;
    for (int i = 1; i<argc; ++i) {
        std::ifstream input(argv[i]);
        u.load(input);
    }

    hcc::asm_program out;

    const std::vector<std::string> regs = {
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
    const int reserved = 256;

    {
        // call Sys.init and halt
        // store return address
        out.emitA("__halt");
        out.emitC(DEST_D | COMP_A);
        out.emitA(reserved);
        out.emitC(DEST_M | COMP_D);
        // set reg_localc for called subroutine
        out.emitA(reserved + regs.size() + 1);
        out.emitC(DEST_D | COMP_A);
        out.emitA(reg_locals);
        out.emitC(DEST_M | COMP_D);
        // jump
        out.emitA("Sys.init");
        out.emitL("__halt");
        out.emitC(COMP_ZERO | JMP);
    }

    {
        out.emitL("__returnHelper");
        out.emitA(reg_locals);
        out.emitC(DEST_D | COMP_M);
        out.emitA(reg_stack_pointer);
        out.emitC(DEST_M | COMP_D);
        for (auto i = regs.begin(), e = regs.end(); i != e; ++i) {
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

    {
        out.emitL("__callHelper");
        for (auto i = regs.rbegin(), e = regs.rend(); i != e; ++i) {
            push(out, *i);
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

    for (auto& subroutine_entry : u.subroutines) {
        subroutine_entry.second.ssa_deconstruct();
        subroutine_entry.second.allocate_registers();
        generate_code(u, subroutine_entry.second, out, subroutine_entry.first);
    }
    out.local_optimization();
    out.save("output.asm");
    return 0;
} catch (const std::runtime_error& e) {
    std::cerr
        << "When executing "
        << boost::algorithm::join(std::vector<std::string>(argv, argv + argc), " ")
        << " ...\n" << e.what();
    return 1;
}
