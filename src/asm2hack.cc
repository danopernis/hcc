// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include <fstream>
#include <iostream>
#include <string>
#include "asm.h"

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Missing input file\n";
        return 1;
    }

    const auto input = std::string { argv[1] };
    const auto output = input.substr(0, input.rfind('.')) + ".hack";

    std::ifstream input_stream { input };
    hcc::asm_program prog { input_stream };
    hcc::saveHACK(output, prog.assemble());

    return 0;
}
