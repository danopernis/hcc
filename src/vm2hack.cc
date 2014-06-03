// Copyright (c) 2012-2014 Dano Pernis
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

    hcc::o_stat_reset();
    for (int i = 1; i<argc; ++i) {
        std::string filename(argv[i]);
        writer.setFilename(filename);
        hcc::VMParser parser(filename);

        hcc::VMCommandList cmds;

        // load commands from file
        while (parser.hasMoreCommands()) {
            hcc::VMCommand c = parser.advance();
            if (c.type == hcc::VMCommand::NOP)
                continue; // NOP is an artifact from parser and thus ignored

            cmds.push_back(c);
        }

        hcc::VMOptimize(cmds);

        for (const auto& c : cmds) {
            writer.write(c);
        }
    }
    hcc::o_stat_print();

    prog.assemble();
    prog.saveHACK("output.hack");

    return 0;
}
