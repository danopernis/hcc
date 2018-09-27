// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include "hcc/vm/command.h"

namespace hcc {
namespace vm {

class parser {
    std::istream& input;
    std::stringstream line;
    command c;

    bool hasMoreCommands();
    command& advance();

public:
    parser(std::istream& input)
        : input(input)
    {
    }

    command_list parse()
    {
        command_list cmds;
        while (hasMoreCommands()) {
            auto c = advance();
            if (c.type != command::NOP) {
                cmds.emplace_back(std::move(c));
            }
        }
        return cmds;
    }
};

} // namespace vm {
} // namespace hcc {
