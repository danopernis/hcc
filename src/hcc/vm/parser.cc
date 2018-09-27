// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/vm/parser.h"

#include <vector>
#include <cstdlib>
#include <stdexcept>

namespace hcc {
namespace vm {

Segment segmentFromString(const std::string& segment)
{
    if (segment.compare("local") == 0) {
        return LOCAL;
    } else if (segment.compare("argument") == 0) {
        return ARGUMENT;
    } else if (segment.compare("this") == 0) {
        return THIS;
    } else if (segment.compare("that") == 0) {
        return THAT;
    } else if (segment.compare("pointer") == 0) {
        return POINTER;
    } else if (segment.compare("temp") == 0) {
        return TEMP;
    } else if (segment.compare("static") == 0) {
        return STATIC;
    }
    throw std::runtime_error("invalid segment name");
}

bool parser::hasMoreCommands()
{
    std::string tmp;
    std::getline(input, tmp);
    line.str(tmp);
    line.clear();
    return input.good();
}

command& parser::advance()
{
    std::vector<std::string> words;
    std::string word;

    while (line.good()) {
        line >> word;
        if (word.find("//") == 0) {
            break;
        } else {
            words.push_back(word);
        }
    }

    if (words.empty()) {
        c.type = command::NOP;
        return c;
    }
    c.in = c.fin = true;

    if (words[0].compare("label") == 0) {
        c.type = command::LABEL;
        c.arg1 = words[1];
        return c;
    } else if (words[0].compare("goto") == 0) {
        c.type = command::GOTO;
        c.arg1 = words[1];
        return c;
    } else if (words[0].compare("if-goto") == 0) {
        c.type = command::IF;
        c.arg1 = words[1];
        c.compare.set(true, false, true);
        return c;
    } else if (words[0].compare("function") == 0) {
        c.type = command::FUNCTION;
        c.arg1 = words[1];
        c.int1 = atoi(words[2].c_str());
        return c;
    } else if (words[0].compare("call") == 0) {
        c.type = command::CALL;
        c.arg1 = words[1];
        c.int1 = atoi(words[2].c_str());
        return c;
    } else if (words[0].compare("return") == 0) {
        c.type = command::RETURN;
        return c;
    } else if (words[0].compare("push") == 0) {
        c.int1 = atoi(words[2].c_str());
        if (words[1].compare("constant") == 0) {
            c.type = command::CONSTANT;
        } else {
            c.type = command::PUSH;
            c.segment1 = segmentFromString(words[1]);
        }
        return c;
    } else if (words[0].compare("pop") == 0) {
        c.int1 = atoi(words[2].c_str());
        c.segment1 = segmentFromString(words[1]);
        switch (c.segment1) {
        case LOCAL:
        case ARGUMENT:
        case THIS:
        case THAT:
            c.type = command::POP_INDIRECT;
            break;
        case POINTER:
        case TEMP:
        case STATIC:
            c.type = command::POP_DIRECT;
            break;
        }
        return c;
    } else if (words[0].compare("add") == 0) {
        c.type = command::BINARY;
        c.binary = ADD;
        return c;
    } else if (words[0].compare("sub") == 0) {
        c.type = command::BINARY;
        c.binary = SUB;
        return c;
    } else if (words[0].compare("neg") == 0) {
        c.type = command::UNARY;
        c.unary = NEG;
        return c;
    } else if (words[0].compare("and") == 0) {
        c.type = command::BINARY;
        c.binary = AND;
        return c;
    } else if (words[0].compare("or") == 0) {
        c.type = command::BINARY;
        c.binary = OR;
        return c;
    } else if (words[0].compare("not") == 0) {
        c.type = command::UNARY;
        c.unary = NOT;
        return c;
    } else if (words[0].compare("lt") == 0) {
        c.type = command::COMPARE;
        c.compare.set(true, false, false);
        return c;
    } else if (words[0].compare("eq") == 0) {
        c.type = command::COMPARE;
        c.compare.set(false, true, false);
        return c;
    } else if (words[0].compare("gt") == 0) {
        c.type = command::COMPARE;
        c.compare.set(false, false, true);
        return c;
    }
    c.type = command::NOP;
    return c;
}

} // namespace vm {
} // namespace hcc {
