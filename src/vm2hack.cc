// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include <iostream>
#include "ParserVM.h"
#include "VMWriter.h"
#include "VMOptimize.h"
#include "asm.h"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Missing input file(s)\n";
        return 1;
    }

    hcc::asm_program prog;
    hcc::VMWriter writer(prog);
    writer.writeBootstrap();

    for (int i = 1; i<argc; ++i) {
        std::string filename(argv[i]);
        std::ifstream input {filename.c_str()};
        hcc::VMParser parser {input};
        auto cmds = parser.parse();
        hcc::VMOptimize(cmds);
        writer.writeFile(filename, cmds);
    }

    hcc::saveHACK("output.hack", prog.assemble());

    return 0;
}
