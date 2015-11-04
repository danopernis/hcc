// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include "VMCommand.h"

namespace hcc {

class VMParser {
    std::istream& input;
    std::stringstream line;
    VMCommand c;

    bool hasMoreCommands();
    VMCommand& advance();

public:
    VMParser(std::istream& input)
        : input(input)
    {
    }

    VMCommandList parse()
    {
        VMCommandList cmds;
        while (hasMoreCommands()) {
            auto c = advance();
            if (c.type != VMCommand::NOP) {
                cmds.emplace_back(std::move(c));
            }
        }
        return cmds;
    }
};

} // namespace hcc {
