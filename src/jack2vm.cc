// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "JackVMWriter.h"
#include "jack_tokenizer.h"
#include "jack_parser.h"
#include <boost/algorithm/string/join.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace hcc::jack;

int main(int argc, char* argv[])
try {
    if (argc < 2) {
        throw std::runtime_error("Missing input file(s)\n");
    }

    // parse input
    std::vector<ast::Class> classes;
    try {
        for (int i = 1; i<argc; ++i) {
            std::cout << "parsing " << argv[i] << '\n';
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

    // TODO semantic analysis

    // produce output
    for (const auto& clazz : classes) {
        VMWriter(clazz.name + ".vm").write(clazz);
    }

    return 0;
} catch (const std::runtime_error& e) {
    std::cerr
        << "When executing "
        << boost::algorithm::join(std::vector<std::string>(argv, argv + argc), " ")
        << " ...\n" << e.what();
    return 1;
}
