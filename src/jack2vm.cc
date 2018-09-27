// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#include "hcc/jack/ast.h"
#include "hcc/jack/parser.h"
#include "hcc/jack/tokenizer.h"
#include "hcc/jack/vm_writer.h"

#include <boost/algorithm/string/join.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char* argv[]) try {
    if (argc < 2) {
        throw std::runtime_error("Missing input file(s)\n");
    }

    // parse input
    std::vector<hcc::jack::Class> classes;
    try {
        for (int i = 1; i < argc; ++i) {
            std::cout << "parsing " << argv[i] << '\n';
            std::ifstream input(argv[i]);
            hcc::jack::tokenizer t{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
            classes.push_back(hcc::jack::parse(t));
        }
    }
    catch (const hcc::jack::parse_error& e) {
        std::stringstream ss;
        ss << "Parse error: " << e.what() << " at " << e.line << ":" << e.column << '\n';
        throw std::runtime_error(ss.str());
    }

    // TODO semantic analysis

    // produce output
    for (const auto& clazz : classes) {
        hcc::jack::VMWriter(clazz.name + ".vm").write(clazz);
    }

    return 0;
}
catch (const std::runtime_error& e) {
    std::cerr << "When executing "
              << boost::algorithm::join(std::vector<std::string>(argv, argv + argc), " ")
              << " ...\n" << e.what();
    return 1;
}
