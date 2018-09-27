// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#include "hcc/assembler/asm.h"
#include "hcc/cpu/cpu.h"
#include "hcc/vm/optimize.h"
#include "hcc/vm/writer.h"
#include "hcc/vm/parser.h"
#include <cassert>
#include <sstream>
#include <vector>

struct driver {
    driver()
        : writer(out)
    {
        writer.writeBootstrap();
    }

    void add_file(const std::string& filename, const std::string& contents)
    {
        std::stringstream input{contents};
        hcc::vm::parser parser{input};
        auto cmds = parser.parse();
        hcc::vm::optimize(cmds);
        writer.writeFile(filename, cmds);
    }

    void run(int ticks = 1000)
    {
        auto instructions = out.assemble();
        hcc::cpu::CPU cpu;
        cpu.reset();
        std::copy(begin(instructions), end(instructions), begin(rom.data));
        for (int i = 0; i < ticks; ++i) {
            cpu.step(&rom, &ram);
        }
    }

    unsigned short get(unsigned int address) const { return ram.get(address); }

private:
    hcc::assembler::program out;
    hcc::vm::writer writer;

    struct RAM : public hcc::cpu::IRAM {
        RAM()
            : data(size, 0)
        {
        }

        void set(unsigned int address, unsigned short value) final { data.at(address) = value; }

        unsigned short get(unsigned int address) const final { return data.at(address); }

        static const unsigned int size = 0x6001;
        std::vector<unsigned short> data;
    } ram;

    struct ROM : public hcc::cpu::IROM {
        ROM()
            : data(size, 0)
        {
        }

        unsigned short get(unsigned int address) const final { return data.at(address); }

        static const unsigned int size = 0x8000;
        std::vector<unsigned short> data;
    } rom;
};

void test_bootstrap()
{
    driver d;
    d.add_file("Sys.vm", R"(
// test bootstrap code
// bootstrap code should setup stack and call Sys.init function
function Sys.init 0
)");
    d.run();
    assert(d.get(0) == 261);
    assert(d.get(1) == 261);
    assert(d.get(2) == 256);
}

void test_stack()
{
    driver d;
    d.add_file("Sys.vm", R"(
// test arithmetic and comparison operations on stack
// adopted from "The Elements of Computing Systems" by Nisan and Schocken, MIT Press
function Sys.init 0
push constant 17
push constant 17
eq
push constant 892
push constant 891
lt
push constant 32767
push constant 32766
gt
push constant 56
push constant 31
push constant 53
add
push constant 112
sub
neg
and
push constant 82
or
)");
    d.run();
    assert(d.get(0) == 265);
    assert(d.get(261) == 65535);
    assert(d.get(262) == 0);
    assert(d.get(263) == 65535);
    assert(d.get(264) == 90);
}

void test_pointer()
{
    driver d;
    d.add_file("Sys.vm", R"(
// test pop&push using this, that and pointer segments
// adopted from "The Elements of Computing Systems" by Nisan and Schocken, MIT Press
function Sys.init 0
push constant 3030
pop pointer 0
push constant 3040
pop pointer 1
push constant 32
pop this 2
push constant 46
pop that 6
push pointer 0
push pointer 1
add
push this 2
sub
push that 6
add
)");
    d.run();
    assert(d.get(261) == 6084);
    assert(d.get(3) == 3030);
    assert(d.get(4) == 3040);
    assert(d.get(3032) == 32);
    assert(d.get(3046) == 46);
}

void test_static()
{
    driver d;
    d.add_file("Sys.vm", R"(
// test pop&push using static segment
// adopted from "The Elements of Computing Systems" by Nisan and Schocken, MIT Press
function Sys.init 0
push constant 111
push constant 333
push constant 888
pop static 8
pop static 3
pop static 1
push static 3
push static 1
sub
push static 8
add
)");
    d.run();
    assert(d.get(261) == 1110);
}

void test_fibonacci()
{
    driver d;
    d.add_file("Main.vm", R"(
// Computes the n'th element of the Fibonacci series, recursively.
// n is given in argument[0].  Called by the Sys.init function
// (part of the Sys.vm file), which also pushes the argument[0]
// parameter before this code starts running.
function Main.fibonacci 0
push argument 0
push constant 2
lt                     // check if n < 2
if-goto IF_TRUE
goto IF_FALSE
label IF_TRUE          // if n<2, return n
push argument 0
return
label IF_FALSE         // if n>=2, return fib(n-2)+fib(n-1)
push argument 0
push constant 2
sub
call Main.fibonacci 1  // compute fib(n-2)
push argument 0
push constant 1
sub
call Main.fibonacci 1  // compute fib(n-1)
add                    // return fib(n-1) + fib(n-2)
return
)");
    d.add_file("Sys.vm", R"(
// Pushes n onto the stack and calls the Main.fibonacii function,
// which computes the n'th element of the Fibonacci series.
// The Sys.init function is called "automatically" by the
// bootstrap code.
function Sys.init 0
push constant 4
call Main.fibonacci 1   // Compute the 4'th fibonacci element
label WHILE
goto WHILE              // Loop infinitely
)");
    d.run();
    assert(d.get(0) == 262);
    assert(d.get(261) == 3);
}

void test_static_multi()
{
    driver d;
    d.add_file("Class1.vm", R"(
// Stores two supplied arguments in static[0] and static[1].
function Class1.set 0
push argument 0
pop static 0
push argument 1
pop static 1
push constant 0
return
// Returns static[0] - static[1].
function Class1.get 0
push static 0
push static 1
sub
return
)");
    d.add_file("Class2.vm", R"(
// Stores two supplied arguments in static[0] and static[1].
function Class2.set 0
push argument 0
pop static 0
push argument 1
pop static 1
push constant 0
return
// Returns static[0] - static[1].
function Class2.get 0
push static 0
push static 1
sub
return
)");
    d.add_file("Sys.vm", R"(
// Tests that different functions, stored in two different
// class files, manipulate the static segment correctly.
function Sys.init 0
push constant 6
push constant 8
call Class1.set 2
pop temp 0 // Dumps the return value
push constant 23
push constant 15
call Class2.set 2
pop temp 0 // Dumps the return value
call Class1.get 0
call Class2.get 0
label WHILE
goto WHILE
)");
    d.run();
    assert(d.get(0) == 263);
    assert(d.get(261) == 65534);
    assert(d.get(262) == 8);
}

int main()
{
    test_bootstrap();
    test_stack();
    test_pointer();
    test_static();
    test_fibonacci();
    test_static_multi();
    return 0;
}
