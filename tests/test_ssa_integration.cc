// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#include "hcc/assembler/asm.h"
#include "hcc/cpu/cpu.h"
#include "hcc/jack/ast.h"
#include "hcc/jack/parser.h"
#include "hcc/jack/tokenizer.h"
#include "hcc/ssa/ssa.h"
#include <sstream>

struct driver {
    driver(const std::string& jack_program, int ticks = 1000)
    {
        // jack
        std::istringstream input{jack_program};
        hcc::jack::tokenizer t{std::istreambuf_iterator<char>(input),
                               std::istreambuf_iterator<char>()};
        auto class_ = hcc::jack::parse(t);

        // ssa
        hcc::ssa::unit u;
        u.translate_from_jack(class_);
        for (auto& subroutine_entry : u.subroutines) {
            auto& subroutine = subroutine_entry.second;
            subroutine.dead_code_elimination();
            subroutine.copy_propagation();
            subroutine.dead_code_elimination();
            subroutine.ssa_deconstruct();
            subroutine.allocate_registers();
        }

        // asm
        hcc::assembler::program out;
        u.translate_to_asm(out);
        out.local_optimization();
        auto instructions = out.assemble();

        // hack
        hcc::cpu::CPU cpu;
        cpu.reset();
        hcc::cpu::ROM rom;
        assert(instructions.size() <= rom.size());
        std::copy(begin(instructions), end(instructions), begin(rom));
        for (int i = 0; i < ticks; ++i) {
            cpu.step(rom, ram);
        }
    }

    hcc::cpu::RAM ram;
};

auto test_store_imm_input = R"(
class Sys {
    static int a;
    function void init()
    {
        let a = 42;
        return 0;
    }
}
)";
void test_store_imm()
{
    driver d{test_store_imm_input};
    assert(d.ram.at(16) == 42);
}

auto test_subroutine_input = R"(
class Sys {
    static int a;
    function void init()
    {
        let a = Sys.return42();
        return 0;
    }
    function int return42()
    {
        return 42;
    }
}
)";
void test_subroutine()
{
    driver d{test_subroutine_input};
    assert(d.ram.at(16) == 42);
}

auto test_arithmetic_input = R"(
class Sys {
    static int a;
    static int b;
    static int c;
    static int d;
    static int e;
    static int f;
    static int g;
    static int h;
    static int i;
    static int j;
    static int k;
    static int l;
    static int m;
    static int n;
    function void init()
    {
        let a = 6;
        let b = 4;
        let c = -a;
        let d = ~a;
        let e = a + b;
        let f = a - b;
        let g = a & b;
        let h = a | b;
        let i = a > b;
        let j = b > a;
        let k = a < b;
        let l = b < a;
        let m = a = a;
        let n = a = b;
        return 0;
    }
}
)";
void test_arithmetic()
{
    driver d{test_arithmetic_input};
    assert(d.ram.at(16) == 6);
    assert(d.ram.at(17) == 4);
    assert(d.ram.at(18) == 65530);
    assert(d.ram.at(19) == 65529);
    assert(d.ram.at(20) == 10);
    assert(d.ram.at(21) == 2);
    assert(d.ram.at(22) == 4);
    assert(d.ram.at(23) == 6);
    assert(d.ram.at(24) == 65535);
    assert(d.ram.at(25) == 0);
    assert(d.ram.at(26) == 0);
    assert(d.ram.at(27) == 65535);
    assert(d.ram.at(28) == 65535);
    assert(d.ram.at(29) == 0);
}

auto test_branching_input = R"(
class Sys {
    static int a;
    function void init()
    {
        while (true) {
            if (false) {
                let a = 0;
                return 0;
            } else {
                let a = 42;
                return 0;
            }
        }
        return 0;
    }
}
)";
void test_branching()
{
    driver d{test_branching_input};
    assert(d.ram.at(16) == 42);
}

auto test_arguments_input = R"(
class Sys {
    static int a;
    function void init()
    {
        let a = Sys.identity(42);
        return 0;
    }
    function int identity(int x)
    {
        return x;
    }
}
)";
void test_arguments()
{
    driver d{test_arguments_input};
    assert(d.ram.at(16) == 42);
}

int main()
{
    test_store_imm();
    test_subroutine();
    test_arithmetic();
    test_branching();
    test_arguments();
    return 0;
}
