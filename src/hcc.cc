// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "asm.h"
#include "jack_parser.h"
#include "jack_tokenizer.h"
#include "ssa.h"
#include <boost/algorithm/string/join.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace hcc::jack;
using namespace hcc::ssa;

int main(int argc, char* argv[])
try {
    if (argc < 2) {
        throw std::runtime_error("Missing input file(s)\n");
    }

    // parse input
    std::vector<ast::Class> classes;
    try {
        for (int i = 1; i<argc; ++i) {
            std::ifstream input(argv[i]);
            tokenizer t {
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>()};
            classes.push_back(parse(t));
        }
    } catch (const parse_error& e) {
        std::stringstream ss;
        ss  << "Parse error: " << e.what()
            << " at " << e.line << ":" << e.column << '\n';
        throw std::runtime_error(ss.str());
    }

    // produce intermediate code
    unit u;
    for (const auto& class_ : classes) {
        u.translate_from_jack(class_);
    }

    // optimize
    for (auto& subroutine_entry : u.subroutines) {
        auto& subroutine = subroutine_entry.second;
        subroutine.dead_code_elimination();
        subroutine.copy_propagation();
        subroutine.dead_code_elimination();
        subroutine.ssa_deconstruct();
        subroutine.allocate_registers();
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
