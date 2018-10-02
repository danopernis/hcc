// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/vm/writer.h"

#include "hcc/cpu/instruction.h"

#include <cstdlib>
#include <map>
#include <sstream>
#include <stdexcept>

namespace hcc {
namespace vm {

using namespace instruction;

writer::writer(assembler::program& out)
    : out(out)
{
    compareCounter = 0;
    returnCounter = 0;
}

const std::string constructString(const std::string s, unsigned int i)
{
    std::stringstream ss;
    ss << s << i;
    return ss.str();
}
const std::string constructString(const std::string s1, const std::string s2)
{
    std::stringstream ss;
    ss << s1 << '$' << s2;
    return ss.str();
}
void writer::push()
{
    out.emitLoadSymbolic("SP");
    out.emitInstruction(DEST_M | COMP_M_PLUS_ONE); // ++SP
    out.emitInstruction(DEST_A | COMP_M_MINUS_ONE);
    out.emitInstruction(DEST_M | COMP_D); // push
}
void writer::pop()
{
    out.emitLoadSymbolic("SP");
    out.emitInstruction(DEST_A | DEST_M | COMP_M_MINUS_ONE); // --SP
    out.emitInstruction(DEST_D | COMP_M); // pop
}
void writer::unaryCompare(int intArg)
{
    switch (intArg) {
    case 0:
        // D=D-0
        break;
    case 1:
        out.emitInstruction(DEST_D | COMP_D_MINUS_ONE); // comparison
        break;
    case -1:
        out.emitInstruction(DEST_D | COMP_D_PLUS_ONE); // comparison
        break;
    default:
        out.emitLoadConstant(intArg);
        out.emitInstruction(DEST_D | COMP_D_MINUS_A); // comparison
        break;
    }
}
/*
 * reg = M[segment] + index
 */
void writer::load(unsigned short dest, Segment segment, unsigned int index)
{
    // some instructions can be saved when index = 0, 1 or 2
    if (index > 2) {
        out.emitLoadConstant(index);
        out.emitInstruction(DEST_D | COMP_A);
    }
    switch (segment) {
    case LOCAL:
        out.emitLoadSymbolic("LCL");
        break;
    case ARGUMENT:
        out.emitLoadSymbolic("ARG");
        break;
    case THIS:
        out.emitLoadSymbolic("THIS");
        break;
    case THAT:
        out.emitLoadSymbolic("THAT");
        break;
    default:
        throw 1;
    }
    switch (index) {
    case 0:
        out.emitInstruction(dest | COMP_M);
        break;
    case 1:
        out.emitInstruction(dest | COMP_M_PLUS_ONE);
        break;
    case 2:
        out.emitInstruction(dest | COMP_M_PLUS_ONE);
        if (dest == DEST_D) {
            out.emitInstruction(DEST_D | COMP_D_PLUS_ONE);
        }
        if (dest == DEST_A) {
            out.emitInstruction(DEST_A | COMP_A_PLUS_ONE);
        }
        break;
    default:
        out.emitInstruction(dest | COMP_D_PLUS_M);
        break;
    }
}
void writer::push_load(Segment segment, int index)
{
    switch (segment) {
    case STATIC:
        out.emitLoadSymbolic(constructString(filename, index));
        out.emitInstruction(DEST_D | COMP_M);
        break;
    case POINTER:
        out.emitLoadConstant(3 + index);
        out.emitInstruction(DEST_D | COMP_M);
        break;
    case TEMP:
        out.emitLoadConstant(5 + index);
        out.emitInstruction(DEST_D | COMP_M);
        break;
    case LOCAL:
    case ARGUMENT:
    case THIS:
    case THAT:
        load(DEST_A, segment, index);
        out.emitInstruction(DEST_D | COMP_M);
        break;
    }
}
void writer::poptop(bool in)
{
    if (in) {
        pop();
        out.emitInstruction(DEST_A | COMP_A_MINUS_ONE);
    } else {
        out.emitLoadSymbolic("SP");
        out.emitInstruction(DEST_A | COMP_M_MINUS_ONE);
    }
}

/*
 * CONSTANT, PUSH, PCOMP_DIRECT, PCOMP_INDIRECT, COPY
 */
void writer::writeConstant(bool, bool fin, int value)
{
    if (fin && -2 <= value && value <= 2) {
        out.emitLoadSymbolic("SP");
        out.emitInstruction(DEST_M | COMP_M_PLUS_ONE); // ++SP
        out.emitInstruction(DEST_A | COMP_M_MINUS_ONE);
        switch (value) {
        case -2:
            out.emitInstruction(DEST_M | COMP_MINUS_ONE);
            out.emitInstruction(DEST_M | COMP_M_MINUS_ONE);
            break;
        case -1:
            out.emitInstruction(DEST_M | COMP_MINUS_ONE);
            break;
        case 0:
            out.emitInstruction(DEST_M | COMP_ZERO);
            break;
        case 1:
            out.emitInstruction(DEST_M | COMP_ONE);
            break;
        case 2:
            out.emitInstruction(DEST_M | COMP_ONE);
            out.emitInstruction(DEST_M | COMP_M_PLUS_ONE);
            break;
        }
        return;
    }

    if (value == -1) {
        out.emitInstruction(DEST_D | COMP_MINUS_ONE);
    } else if (value == 0) {
        out.emitInstruction(DEST_D | COMP_ZERO);
    } else if (value == 1) {
        out.emitInstruction(DEST_D | COMP_ONE);
    } else if (value < 0) {
        out.emitLoadConstant(-value);
        out.emitInstruction(DEST_D | COMP_MINUS_A);
    } else {
        out.emitLoadConstant(value);
        out.emitInstruction(DEST_D | COMP_A);
    }
    if (fin)
        push();
}
void writer::writePush(bool, bool fin, Segment segment, int index)
{
    push_load(segment, index);
    if (fin)
        push();
}
void writer::writePopDirect(bool in, bool, Segment segment, int index)
{
    if (in)
        pop();
    switch (segment) {
    case STATIC:
        out.emitLoadSymbolic(constructString(filename.c_str(), index));
        break;
    case POINTER:
        out.emitLoadConstant(3 + index);
        break;
    case TEMP:
        out.emitLoadConstant(5 + index);
        break;
    case LOCAL:
    case ARGUMENT:
    case THIS:
    case THAT:
        throw 1;
    }
    out.emitInstruction(DEST_M | COMP_D); // save
}
void writer::writePopIndirect(Segment segment, int index)
{
    load(DEST_D, segment, index);
    out.emitLoadSymbolic("R15");
    out.emitInstruction(DEST_M | COMP_D); // save calculated address for popping
    pop();
    out.emitLoadSymbolic("R15");
    out.emitInstruction(DEST_A | COMP_M); // load the address
    out.emitInstruction(DEST_M | COMP_D); // save
}
void writer::writePopIndirectPush(bool, bool fin, Segment segment, int index)
{
    load(DEST_D, segment, index);
    out.emitLoadSymbolic("R15");
    out.emitInstruction(DEST_M | COMP_D); // save calculated address for popping
    pop();
    out.emitLoadSymbolic("R15");
    out.emitInstruction(DEST_A | COMP_M); // load the address
    out.emitInstruction(DEST_M | COMP_D); // save
    if (fin)
        push();
}
void writer::writeCopy(Segment sseg, int sind, Segment dseg, int dind)
{
    if (sseg == dseg) {
        if (sind == dind)
            return; // no need to copy

        if (std::abs(dind - sind) < 4) { // 4 is empiric constant
            load(DEST_A, sseg, sind);
            out.emitInstruction(DEST_D | COMP_M); // fetch
            for (int i = sind; i > dind; --i)
                out.emitInstruction(DEST_A | COMP_A_MINUS_ONE);
            for (int i = sind; i < dind; ++i)
                out.emitInstruction(DEST_A | COMP_A_PLUS_ONE);
            out.emitInstruction(DEST_M | COMP_D); // store
            return;
        }
    }

    // TODO: do not duplicate writePop()
    load(DEST_D, dseg, dind);
    out.emitLoadSymbolic("R15");
    out.emitInstruction(DEST_M | COMP_D); // save calculated address for popping
    push_load(sseg, sind);
    out.emitLoadSymbolic("R15");
    out.emitInstruction(DEST_A | COMP_M); // load the address
    out.emitInstruction(DEST_M | COMP_D); // copy
}
/*
 * UNARY, BINARY
 */
void writer::writeUnary(bool in, bool fin, UnaryOperation op, int intArg)
{
    if (in && fin) {
        std::map<UnaryOperation, unsigned short> unaryTable;
        unaryTable[NOT] = DEST_M | COMP_NOT_M;
        unaryTable[NEG] = DEST_M | COMP_MINUS_M;
        unaryTable[ADDC] = DEST_M | COMP_D_PLUS_M;
        unaryTable[SUBC] = DEST_M | COMP_M_MINUS_D;
        unaryTable[BUSC] = DEST_M | COMP_D_MINUS_M;
        unaryTable[ANDC] = DEST_M | COMP_D_AND_M;
        unaryTable[ORC] = DEST_M | COMP_D_OR_M;

        switch (op) {
        case NOT:
        case NEG:
            out.emitLoadSymbolic("SP");
            out.emitInstruction(DEST_A | COMP_M_MINUS_ONE);
            out.emitInstruction(unaryTable[op]);
            break;
        case DOUBLE:
            out.emitInstruction(DEST_D | COMP_M);
            out.emitInstruction(DEST_M | COMP_D_PLUS_M);
            break;
        case SUBC:
            if (intArg == 1) {
                out.emitLoadSymbolic("SP");
                out.emitInstruction(DEST_A | COMP_M_MINUS_ONE);
                out.emitInstruction(DEST_M | COMP_M_MINUS_ONE);
                return;
            }
        // no break;
        case ADDC:
        case BUSC:
        case ANDC:
        case ORC:
            out.emitLoadConstant(intArg);
            out.emitInstruction(DEST_D | COMP_A);
            out.emitLoadSymbolic("SP");
            out.emitInstruction(DEST_A | COMP_M_MINUS_ONE);
            out.emitInstruction(unaryTable[op]);
            break;
        }

    } else {
        if (in)
            pop();
        switch (op) {
        case NOT:
            out.emitInstruction(DEST_D | COMP_NOT_D);
            break;
        case NEG:
            out.emitInstruction(DEST_D | COMP_MINUS_D);
            break;
        case DOUBLE:
            out.emitInstruction(DEST_A | COMP_D);
            out.emitInstruction(DEST_D | COMP_D_PLUS_A);
            break;
        case ADDC:
            if (intArg == 1) {
                out.emitInstruction(DEST_D | COMP_D_PLUS_ONE);
            } else {
                out.emitLoadConstant(intArg);
                out.emitInstruction(DEST_D | COMP_D_PLUS_A);
            }
            break;
        case SUBC:
            if (intArg == 1) {
                out.emitInstruction(DEST_D | COMP_D_MINUS_ONE);
            } else {
                out.emitLoadConstant(intArg);
                out.emitInstruction(DEST_D | COMP_D_MINUS_A);
            }
            break;
        case BUSC:
            out.emitLoadConstant(intArg);
            out.emitInstruction(DEST_D | COMP_A_MINUS_D);
            break;
        case ANDC:
            out.emitLoadConstant(intArg);
            out.emitInstruction(DEST_D | COMP_D_AND_A);
            break;
        case ORC:
            out.emitLoadConstant(intArg);
            out.emitInstruction(DEST_D | COMP_D_OR_A);
            break;
        }
        if (fin)
            push();
    }
}
void writer::writeBinary(bool in, bool fin, BinaryOperation op)
{
    unsigned short dest;
    if (fin) { // store result to memory; do not adjust SP
        dest = DEST_M;
        poptop(in);
    } else { // store result to register; adjust SP
        dest = DEST_D;
        if (in) // fetch argument from stack
            pop();
        out.emitLoadSymbolic("SP");
        out.emitInstruction(DEST_A | DEST_M | COMP_M_MINUS_ONE);
    }
    switch (op) {
    case ADD:
        out.emitInstruction(dest | COMP_D_PLUS_M);
        break;
    case SUB:
        out.emitInstruction(dest | COMP_M_MINUS_D);
        break;
    case BUS:
        out.emitInstruction(dest | COMP_D_MINUS_M);
        break;
    case AND:
        out.emitInstruction(dest | COMP_D_AND_M);
        break;
    case OR:
        out.emitInstruction(dest | COMP_D_OR_M);
        break;
    }
}
/*
 * UNARY_COMPARE, COMPARE
 */
void writer::compareBranches(bool fin, CompareOperation op)
{
    std::string compareSwitch = constructString("__compareSwitch", compareCounter);
    std::string compareEnd = constructString("__compareEnd", compareCounter);
    ++compareCounter;

    if (fin) {
        out.emitLoadSymbolic(compareEnd);
        out.emitInstruction(COMP_D | op.jump());
        out.emitLoadSymbolic("SP");
        out.emitInstruction(DEST_A | COMP_M_MINUS_ONE);
        out.emitInstruction(DEST_M | COMP_ZERO); // adjust to false
        out.emitLabel(compareEnd);
    } else {
        out.emitLoadSymbolic(compareSwitch);
        out.emitInstruction(COMP_D | op.jump());
        out.emitInstruction(DEST_D | COMP_ZERO);
        out.emitLoadSymbolic(compareEnd);
        out.emitInstruction(COMP_ZERO | JMP);
        out.emitLabel(compareSwitch);
        out.emitInstruction(DEST_D | COMP_MINUS_ONE);
        out.emitLabel(compareEnd);
    }
}
void writer::writeUnaryCompare(bool in, bool fin, CompareOperation op, int intArg)
{
    if (fin) { // save result to memory
        if (in) { // fetch argument from stack, do not adjust SP
            out.emitLoadSymbolic("SP");
            out.emitInstruction(DEST_A | COMP_M_MINUS_ONE);
            out.emitInstruction(DEST_D | COMP_M);
        } else { // argument is in register, increment SP
            out.emitLoadSymbolic("SP");
            out.emitInstruction(DEST_M | COMP_M_PLUS_ONE);
            out.emitInstruction(DEST_A | COMP_M_MINUS_ONE);
        }
        out.emitInstruction(DEST_M | COMP_MINUS_ONE); // default is true
    } else { // save result to register
        if (in)
            pop(); // fetch argument from stack
    }
    unaryCompare(intArg);
    compareBranches(fin, op);
}
void writer::writeCompare(bool in, bool fin, CompareOperation op)
{
    poptop(in);
    out.emitInstruction(DEST_D | COMP_M_MINUS_D); // comparison
    if (fin) {
        out.emitInstruction(DEST_M | COMP_MINUS_ONE); // default is true
    } else {
        out.emitLoadSymbolic("SP");
        out.emitInstruction(DEST_M | COMP_M_MINUS_ONE);
    }
    compareBranches(fin, op);
}
/*
 * LABEL, GOTO, IF, COMPARE_IF, UNARY_COMPARE_IF
 */
void writer::writeLabel(const std::string label) { out.emitLabel(constructString(function, label)); }
void writer::writeGoto(const std::string label)
{
    out.emitLoadSymbolic(constructString(function, label));
    out.emitInstruction(COMP_ZERO | JMP);
}
void writer::writeIf(bool in, bool, CompareOperation op, const std::string label,
                       bool compare, bool useConst, int intConst)
{
    if (in)
        pop();
    if (compare) {
        if (useConst) { // UNARY_COMPARE_IF
            unaryCompare(intConst);
        } else { // COMPARE_IF
            out.emitLoadSymbolic("SP");
            out.emitInstruction(DEST_A | DEST_M | COMP_M_MINUS_ONE); // --SP
            out.emitInstruction(DEST_D | COMP_M_MINUS_D); // comparison
        }
    } // else just IF
    out.emitLoadSymbolic(constructString(function, label));
    out.emitInstruction(COMP_D | op.jump());
}
/*
 * FUNCTION, CALL, RETURN
 */
void writer::writeFunction(const std::string name, int localc)
{
    function = name;
    out.emitLabel(name);
    switch (localc) {
    case 0:
        break;
    case 1:
        out.emitLoadSymbolic("SP");
        out.emitInstruction(DEST_M | COMP_M_PLUS_ONE);
        out.emitInstruction(DEST_A | COMP_M_MINUS_ONE);
        out.emitInstruction(DEST_M | COMP_ZERO);
        break;
    default: // 2*localc+4 instructions
        out.emitLoadSymbolic("SP");
        out.emitInstruction(DEST_A | COMP_M);
        for (int i = 1; i < localc; ++i) {
            out.emitInstruction(DEST_M | COMP_ZERO);
            out.emitInstruction(DEST_A | COMP_A_PLUS_ONE);
        }
        out.emitInstruction(DEST_M | COMP_ZERO);
        out.emitInstruction(DEST_D | COMP_A_PLUS_ONE); // "unroll"
        out.emitLoadSymbolic("SP");
        out.emitInstruction(DEST_M | COMP_D);
        break;
    }
}
// 44 instructions when called for the first time
//  8 instructions when called next time with the same argc
void writer::writeCall(const std::string name, int argc)
{
    bool found = false;
    for (std::list<int>::iterator i = argStubs.begin(); i != argStubs.end(); ++i) {
        if (*i == argc) {
            found = true;
        }
    }

    std::string call = constructString("__call", argc);
    std::string returnAddress = constructString("__returnAddress", returnCounter);
    ++returnCounter;

    out.emitLoadSymbolic(name);
    out.emitInstruction(DEST_D | COMP_A);
    out.emitLoadSymbolic("R15");
    out.emitInstruction(DEST_M | COMP_D);
    out.emitLoadSymbolic(returnAddress);
    out.emitInstruction(DEST_D | COMP_A);
    if (found) {
        out.emitLoadSymbolic(call);
        out.emitInstruction(COMP_ZERO | JMP);
    } else {
        out.emitLabel(call);
        push();
        out.emitLoadSymbolic("LCL");
        out.emitInstruction(DEST_D | COMP_M);
        push();
        out.emitLoadSymbolic("ARG");
        out.emitInstruction(DEST_D | COMP_M);
        push();
        out.emitLoadSymbolic("THIS");
        out.emitInstruction(DEST_D | COMP_M);
        push();
        out.emitLoadSymbolic("THAT");
        out.emitInstruction(DEST_D | COMP_M);
        push();
        out.emitLoadSymbolic("SP");
        out.emitInstruction(DEST_D | COMP_M);
        out.emitLoadSymbolic("LCL");
        out.emitInstruction(DEST_M | COMP_D); // LCL = SP
        out.emitLoadConstant(argc + 5);
        out.emitInstruction(DEST_D | COMP_D_MINUS_A);
        out.emitLoadSymbolic("ARG");
        out.emitInstruction(DEST_M | COMP_D); // ARG = SP - " << argc << " - 5\n"
        out.emitLoadSymbolic("R15");
        out.emitInstruction(DEST_A | COMP_M | JMP);
        argStubs.push_back(argc);
    }
    out.emitLabel(returnAddress);
}

void writer::writeReturn()
{
    out.emitLoadSymbolic("__return");
    out.emitInstruction(COMP_ZERO | JMP);
}
/*
 * BOOTSTRAP
 */
void writer::writeBootstrap()
{
    out.emitLoadConstant(256);
    out.emitInstruction(DEST_D | COMP_A);
    out.emitLoadSymbolic("SP");
    out.emitInstruction(DEST_M | COMP_D);
    writeCall("Sys.init", 0);
    out.emitLabel("__return");
    out.emitLoadConstant(5);
    out.emitInstruction(DEST_D | COMP_A);
    out.emitLoadSymbolic("LCL");
    out.emitInstruction(DEST_A | COMP_M_MINUS_D);
    out.emitInstruction(DEST_D | COMP_M);
    out.emitLoadSymbolic("R15");
    out.emitInstruction(DEST_M | COMP_D); // R15 = *(LCL-5)
    out.emitLoadSymbolic("SP");
    out.emitInstruction(DEST_A | DEST_M | COMP_M_MINUS_ONE);
    out.emitInstruction(DEST_D | COMP_M); // return value
    out.emitLoadSymbolic("ARG");
    out.emitInstruction(DEST_A | COMP_M);
    out.emitInstruction(DEST_M | COMP_D); // *ARG = pop()
    out.emitInstruction(DEST_D | COMP_A_PLUS_ONE);
    out.emitLoadSymbolic("SP");
    out.emitInstruction(DEST_M | COMP_D); // SP = ARG + 1
    out.emitLoadSymbolic("LCL");
    out.emitInstruction(DEST_D | COMP_M);
    out.emitLoadSymbolic("R14");
    out.emitInstruction(DEST_A | DEST_M | COMP_D_MINUS_ONE); // R14 = LCL - 1
    out.emitInstruction(DEST_D | COMP_M);
    out.emitLoadSymbolic("THAT");
    out.emitInstruction(DEST_M | COMP_D); // THAT = M[R14]
    out.emitLoadSymbolic("R14");
    out.emitInstruction(DEST_A | DEST_M | COMP_M_MINUS_ONE);
    out.emitInstruction(DEST_D | COMP_M);
    out.emitLoadSymbolic("THIS");
    out.emitInstruction(DEST_M | COMP_D); // THIS = M[R14--]
    out.emitLoadSymbolic("R14");
    out.emitInstruction(DEST_A | DEST_M | COMP_M_MINUS_ONE);
    out.emitInstruction(DEST_D | COMP_M);
    out.emitLoadSymbolic("ARG");
    out.emitInstruction(DEST_M | COMP_D); // ARG = M[R14--]
    out.emitLoadSymbolic("R14");
    out.emitInstruction(DEST_A | DEST_M | COMP_M_MINUS_ONE);
    out.emitInstruction(DEST_D | COMP_M);
    out.emitLoadSymbolic("LCL");
    out.emitInstruction(DEST_M | COMP_D); // LCL = M[R14--]
    out.emitLoadSymbolic("R15");
    out.emitInstruction(DEST_A | COMP_M | JMP); // goto R15
}

void writer::write(const command& c)
{
    {
        std::stringstream ss;
        ss << c;
        out.emitComment(ss.str());
    }
    switch (c.type) {
    case command::CONSTANT:
        writeConstant(c.in, c.fin, c.int1);
        break;
    case command::PUSH:
        writePush(c.in, c.fin, c.segment1, c.int1);
        break;
    case command::POP_DIRECT:
        writePopDirect(c.in, c.fin, c.segment1, c.int1);
        break;
    case command::POP_INDIRECT:
        writePopIndirect(c.segment1, c.int1);
        break;
    case command::POP_INDIRECT_PUSH:
        writePopIndirectPush(c.in, c.fin, c.segment1, c.int1);
        break;
    case command::COPY:
        writeCopy(c.segment1, c.int1, c.segment2, c.int2);
        break;
    case command::UNARY:
        writeUnary(c.in, c.fin, c.unary, c.int1);
        break;
    case command::BINARY:
        writeBinary(c.in, c.fin, c.binary);
        break;
    case command::COMPARE:
        writeCompare(c.in, c.fin, c.compare);
        break;
    case command::UNARY_COMPARE:
        writeUnaryCompare(c.in, c.fin, c.compare, c.int1);
        break;
    case command::LABEL:
        writeLabel(c.arg1);
        break;
    case command::GOTO:
        writeGoto(c.arg1);
        break;
    case command::IF:
        writeIf(c.in, c.fin, c.compare, c.arg1, false, false, 0);
        break;
    case command::COMPARE_IF:
        writeIf(c.in, c.fin, c.compare, c.arg1, true, false, 0);
        break;
    case command::UNARY_COMPARE_IF:
        writeIf(c.in, c.fin, c.compare, c.arg1, true, true, c.int1);
        break;
    case command::FUNCTION:
        writeFunction(c.arg1, c.int1);
        break;
    case command::CALL:
        writeCall(c.arg1, c.int1);
        break;
    case command::RETURN:
        writeReturn();
        break;
    case command::NOP:
    case command::IN:
    case command::FIN:
        throw std::runtime_error("helper commands made it to the final version");
        break;
    }
}

} // namespace vm {
} // namespace hcc {
