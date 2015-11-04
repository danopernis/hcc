// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "VMCommand.h"
#include <iostream>
#include <map>

namespace hcc {

std::ostream& operator<<(std::ostream& out, const VMCommand& c)
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
    case VMCommand::CONSTANT:
        out << "//* push constant " << c.int1 << "\n";
        break;
    case VMCommand::PUSH:
        out << "//* push " << segmentVMNames[c.segment1] << " " << c.int1 << "\n";
        break;
    case VMCommand::POP_DIRECT:
    case VMCommand::POP_INDIRECT:
        out << "//* pop " << segmentVMNames[c.segment1] << " " << c.int1 << "\n";
        break;
    case VMCommand::POP_INDIRECT_PUSH:
        out << "//* pop " << segmentVMNames[c.segment1] << " " << c.int1 << "\n"
                                                                            "//* push "
            << segmentVMNames[c.segment1] << " " << c.int1 << "\n";
        break;
    case VMCommand::COPY:
        out << "//* push " << segmentVMNames[c.segment1] << " " << c.int1 << "\n"
                                                                             "//* pop "
            << segmentVMNames[c.segment2] << " " << c.int2 << "\n";
        break;
    case VMCommand::UNARY:
        out << "//* UNARY\n"; // TODO
        break;
    case VMCommand::BINARY:
        out << "//* BINARY\n"; // TODO
        break;
    case VMCommand::COMPARE:
        out << "//* COMPARE\n"; // TODO
        break;
    case VMCommand::UNARY_COMPARE:
        out << "//* push constant " << c.int1 << "\n"
                                                 "//* COMPARE\n"; // TODO
        break;
    case VMCommand::IF:
        out << "//* if-goto " << c.arg1 << "\n";
        break;
    case VMCommand::COMPARE_IF:
        out << "//* COMPARE\n" // TODO
               "//* if-goto " << c.arg1 << "\n";
        break;
    case VMCommand::UNARY_COMPARE_IF:
        out << "//* push constant " << c.int1 << "\n"
                                                 "//* COMPARE\n" // TODO
                                                 "//* if-goto " << c.arg1 << "\n";
        break;
    case VMCommand::LABEL:
        out << "//* label " << c.arg1 << "\n";
        break;
    case VMCommand::GOTO:
        out << "//* goto " << c.arg1 << "\n";
        break;
    case VMCommand::FUNCTION:
        out << "//* function " << c.arg1 << " " << c.int1 << "\n";
        break;
    case VMCommand::CALL:
        out << "//* call " << c.arg1 << " " << c.int1 << "\n";
        break;
    case VMCommand::RETURN:
        out << "//* return\n";
        break;
    case VMCommand::NOP:
    case VMCommand::IN:
    case VMCommand::FIN:
        break;
    }
    return out;
}

} // end namespace
