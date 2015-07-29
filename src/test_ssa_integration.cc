// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "CPU.h"
#include "asm.h"
#include "jack_parser.h"
#include "jack_tokenizer.h"
#include "ssa.h"
#include <sstream>
#include <vector>

struct driver {
    driver(const std::string& jack_program, int ticks = 1000)
    {
        // jack
        std::istringstream input {jack_program};
        hcc::jack::tokenizer t {
            std::istreambuf_iterator<char>(input),
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
        hcc::asm_program out;
        u.translate_to_asm(out);
        out.local_optimization();
        auto instructions = out.assemble();

        // hack
        hcc::CPU cpu;
        cpu.reset();
        std::copy(begin(instructions), end(instructions), begin(rom.data));
        for (int i = 0; i<ticks; ++i) {
            cpu.step(&rom, &ram);
        }
    }

    unsigned short get(unsigned int address) const
    { return ram.get(address); }

private:
    struct RAM : public hcc::IRAM {
        RAM() : data(size, 0) { }

        void set(unsigned int address, unsigned short value) final
        { data.at(address) = value; }

        unsigned short get(unsigned int address) const final
        { return data.at(address); }

        static const unsigned int size = 0x6001;
        std::vector<unsigned short> data;
    } ram;

    struct ROM : public hcc::IROM {
        ROM() : data(size, 0) { }

        unsigned short get(unsigned int address) const final
        { return data.at(address); }

        static const unsigned int size = 0x8000;
        std::vector<unsigned short> data;
    } rom;
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
    driver d {test_store_imm_input};
    assert (d.get(16) == 42);
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
    driver d {test_subroutine_input};
    assert (d.get(16) == 42);
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
    driver d {test_arithmetic_input};
    assert (d.get(16) == 6);
    assert (d.get(17) == 4);
    assert (d.get(18) == 65530);
    assert (d.get(19) == 65529);
    assert (d.get(20) == 10);
    assert (d.get(21) == 2);
    assert (d.get(22) == 4);
    assert (d.get(23) == 6);
    assert (d.get(24) == 65535);
    assert (d.get(25) == 0);
    assert (d.get(26) == 0);
    assert (d.get(27) == 65535);
    assert (d.get(28) == 65535);
    assert (d.get(29) == 0);
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
    driver d {test_branching_input};
    assert (d.get(16) == 42);
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
    driver d {test_arguments_input};
    assert (d.get(16) == 42);
}


int main(int argc, char *argv[])
{
    test_store_imm();
    test_subroutine();
    test_arithmetic();
    test_branching();
    test_arguments();
    return 0;
}
