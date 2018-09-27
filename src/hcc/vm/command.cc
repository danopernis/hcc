// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/vm/command.h"

#include <iostream>
#include <map>

namespace hcc {
namespace vm {

std::ostream& operator<<(std::ostream& out, const command& c)
{
    std::map<Segment, std::string> segmentVMNames;
    segmentVMNames[STATIC] = "static";
    segmentVMNames[POINTER] = "pointer";
    segmentVMNames[TEMP] = "temp";
    segmentVMNames[LOCAL] = "local";
    segmentVMNames[ARGUMENT] = "argument";
    segmentVMNames[THIS] = "this";
    segmentVMNames[THAT] = "that";

    out << "\n";
    switch (c.type) {
    case command::CONSTANT:
        out << "//* push constant " << c.int1 << "\n";
        break;
    case command::PUSH:
        out << "//* push " << segmentVMNames[c.segment1] << " " << c.int1 << "\n";
        break;
    case command::POP_DIRECT:
    case command::POP_INDIRECT:
        out << "//* pop " << segmentVMNames[c.segment1] << " " << c.int1 << "\n";
        break;
    case command::POP_INDIRECT_PUSH:
        out << "//* pop " << segmentVMNames[c.segment1] << " " << c.int1 << "\n"
                                                                            "//* push "
            << segmentVMNames[c.segment1] << " " << c.int1 << "\n";
        break;
    case command::COPY:
        out << "//* push " << segmentVMNames[c.segment1] << " " << c.int1 << "\n"
                                                                             "//* pop "
            << segmentVMNames[c.segment2] << " " << c.int2 << "\n";
        break;
    case command::UNARY:
        out << "//* UNARY\n"; // TODO
        break;
    case command::BINARY:
        out << "//* BINARY\n"; // TODO
        break;
    case command::COMPARE:
        out << "//* COMPARE\n"; // TODO
        break;
    case command::UNARY_COMPARE:
        out << "//* push constant " << c.int1 << "\n"
                                                 "//* COMPARE\n"; // TODO
        break;
    case command::IF:
        out << "//* if-goto " << c.arg1 << "\n";
        break;
    case command::COMPARE_IF:
        out << "//* COMPARE\n" // TODO
               "//* if-goto " << c.arg1 << "\n";
        break;
    case command::UNARY_COMPARE_IF:
        out << "//* push constant " << c.int1 << "\n"
                                                 "//* COMPARE\n" // TODO
                                                 "//* if-goto " << c.arg1 << "\n";
        break;
    case command::LABEL:
        out << "//* label " << c.arg1 << "\n";
        break;
    case command::GOTO:
        out << "//* goto " << c.arg1 << "\n";
        break;
    case command::FUNCTION:
        out << "//* function " << c.arg1 << " " << c.int1 << "\n";
        break;
    case command::CALL:
        out << "//* call " << c.arg1 << " " << c.int1 << "\n";
        break;
    case command::RETURN:
        out << "//* return\n";
        break;
    case command::NOP:
    case command::IN:
    case command::FIN:
        break;
    }
    return out;
}

} // namespace vm` {
} // namespace hcc {
