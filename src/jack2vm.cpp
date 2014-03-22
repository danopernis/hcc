// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#include <sstream>
#include <iostream>
#include <boost/algorithm/string/join.hpp>
#include "JackParser.h"
#include "JackVMWriter.h"

using namespace hcc::jack;

int main(int argc, char* argv[])
try {
    if (argc < 2) {
        throw std::runtime_error("Missing input file(s)\n");
    }

    // parse input
    std::vector<ast::Class> classes;
    try {
        Parser parser;
        for (int i = 1; i<argc; ++i) {
            std::cout << "parsing " << argv[i] << '\n';
            classes.push_back(parser.parse(argv[i]));
        }
    } catch (const ParseError& e) {
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
