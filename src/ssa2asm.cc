// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "asm.h"
#include "ssa.h"
#include <boost/algorithm/string/join.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>

int main(int argc, char* argv[])
try {
    if (argc < 2) {
        throw std::runtime_error("Usage: ssa2asm input...\n");
    }

    hcc::ssa::unit u;
    for (int i = 1; i<argc; ++i) {
        std::ifstream input(argv[i]);
        u.load(input);
    }
    for (auto& subroutine_entry : u.subroutines) {
        subroutine_entry.second.ssa_deconstruct();
        subroutine_entry.second.allocate_registers();
    }

    hcc::asm_program out;
    u.translate_to_asm(out);
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
