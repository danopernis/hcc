// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "VMWriter.h"
#include "instruction.h"
#include <map>
#include <cstdlib>
#include <stdexcept>

namespace hcc {

using namespace instruction;

VMWriter::VMWriter(asm_program& out)
    : out(out)
{
    compareCounter = 0;
    returnCounter = 0;
}

VMWriter::~VMWriter() {}

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
void VMWriter::push()
{
    out.emitA("SP");
    out.emitC(DEST_M | COMP_M_PLUS_ONE); // ++SP
    out.emitC(DEST_A | COMP_M_MINUS_ONE);
    out.emitC(DEST_M | COMP_D); // push
}
void VMWriter::pop()
{
    out.emitA("SP");
    out.emitC(DEST_A | DEST_M | COMP_M_MINUS_ONE); // --SP
    out.emitC(DEST_D | COMP_M); // pop
}
void VMWriter::unaryCompare(int intArg)
{
    switch (intArg) {
    case 0:
        // D=D-0
        break;
    case 1:
        out.emitC(DEST_D | COMP_D_MINUS_ONE); // comparison
        break;
    case -1:
        out.emitC(DEST_D | COMP_D_PLUS_ONE); // comparison
        break;
    default:
        out.emitA(intArg);
        out.emitC(DEST_D | COMP_D_MINUS_A); // comparison
        break;
    }
}
/*
 * reg = M[segment] + index
 */
void VMWriter::load(unsigned short dest, Segment segment, unsigned int index)
{
    // some instructions can be saved when index = 0, 1 or 2
    if (index > 2) {
        out.emitA(index);
        out.emitC(DEST_D | COMP_A);
    }
    switch (segment) {
    case LOCAL:
        out.emitA("LCL");
        break;
    case ARGUMENT:
        out.emitA("ARG");
        break;
    case THIS:
        out.emitA("THIS");
        break;
    case THAT:
        out.emitA("THAT");
        break;
    default:
        throw 1;
    }
    switch (index) {
    case 0:
        out.emitC(dest | COMP_M);
        break;
    case 1:
        out.emitC(dest | COMP_M_PLUS_ONE);
        break;
    case 2:
        out.emitC(dest | COMP_M_PLUS_ONE);
        if (dest == DEST_D) {
            out.emitC(DEST_D | COMP_D_PLUS_ONE);
        }
        if (dest == DEST_A) {
            out.emitC(DEST_A | COMP_A_PLUS_ONE);
        }
        break;
    default:
        out.emitC(dest | COMP_D_PLUS_M);
        break;
    }
}
void VMWriter::push_load(Segment segment, int index)
{
    switch (segment) {
    case STATIC:
        out.emitA(constructString(filename, index));
        out.emitC(DEST_D | COMP_M);
        break;
    case POINTER:
        out.emitA(3 + index);
        out.emitC(DEST_D | COMP_M);
        break;
    case TEMP:
        out.emitA(5 + index);
        out.emitC(DEST_D | COMP_M);
        break;
    case LOCAL:
    case ARGUMENT:
    case THIS:
    case THAT:
        load(DEST_A, segment, index);
        out.emitC(DEST_D | COMP_M);
        break;
    }
}
void VMWriter::poptop(bool in)
{
    if (in) {
        pop();
        out.emitC(DEST_A | COMP_A_MINUS_ONE);
    } else {
        out.emitA("SP");
        out.emitC(DEST_A | COMP_M_MINUS_ONE);
    }
}

/*
 * CONSTANT, PUSH, PCOMP_DIRECT, PCOMP_INDIRECT, COPY
 */
void VMWriter::writeConstant(bool in, bool fin, int value)
{
    if (fin && -2 <= value && value <= 2) {
        out.emitA("SP");
        out.emitC(DEST_M | COMP_M_PLUS_ONE); // ++SP
        out.emitC(DEST_A | COMP_M_MINUS_ONE);
        switch (value) {
        case -2:
            out.emitC(DEST_M | COMP_MINUS_ONE);
            out.emitC(DEST_M | COMP_M_MINUS_ONE);
            break;
        case -1:
            out.emitC(DEST_M | COMP_MINUS_ONE);
            break;
        case 0:
            out.emitC(DEST_M | COMP_ZERO);
            break;
        case 1:
            out.emitC(DEST_M | COMP_ONE);
            break;
        case 2:
            out.emitC(DEST_M | COMP_ONE);
            out.emitC(DEST_M | COMP_M_PLUS_ONE);
            break;
        }
        return;
    }

    if (value == -1) {
        out.emitC(DEST_D | COMP_MINUS_ONE);
    } else if (value == 0) {
        out.emitC(DEST_D | COMP_ZERO);
    } else if (value == 1) {
        out.emitC(DEST_D | COMP_ONE);
    } else if (value < 0) {
        out.emitA(-value);
        out.emitC(DEST_D | COMP_MINUS_A);
    } else {
        out.emitA(value);
        out.emitC(DEST_D | COMP_A);
    }
    if (fin)
        push();
}
void VMWriter::writePush(bool in, bool fin, Segment segment, int index)
{
    push_load(segment, index);
    if (fin)
        push();
}
void VMWriter::writePopDirect(bool in, bool fin, Segment segment, int index)
{
    if (in)
        pop();
    switch (segment) {
    case STATIC:
        out.emitA(constructString(filename.c_str(), index));
        break;
    case POINTER:
        out.emitA(3 + index);
        break;
    case TEMP:
        out.emitA(5 + index);
        break;
    case LOCAL:
    case ARGUMENT:
    case THIS:
    case THAT:
        throw 1;
    }
    out.emitC(DEST_M | COMP_D); // save
}
void VMWriter::writePopIndirect(Segment segment, int index)
{
    load(DEST_D, segment, index);
    out.emitA("R15");
    out.emitC(DEST_M | COMP_D); // save calculated address for popping
    pop();
    out.emitA("R15");
    out.emitC(DEST_A | COMP_M); // load the address
    out.emitC(DEST_M | COMP_D); // save
}
void VMWriter::writePopIndirectPush(bool in, bool fin, Segment segment, int index)
{
    load(DEST_D, segment, index);
    out.emitA("R15");
    out.emitC(DEST_M | COMP_D); // save calculated address for popping
    pop();
    out.emitA("R15");
    out.emitC(DEST_A | COMP_M); // load the address
    out.emitC(DEST_M | COMP_D); // save
    if (fin)
        push();
}
void VMWriter::writeCopy(Segment sseg, int sind, Segment dseg, int dind)
{
    if (sseg == dseg) {
        if (sind == dind)
            return; // no need to copy

        if (std::abs(dind - sind) < 4) { // 4 is empiric constant
            load(DEST_A, sseg, sind);
            out.emitC(DEST_D | COMP_M); // fetch
            for (int i = sind; i > dind; --i)
                out.emitC(DEST_A | COMP_A_MINUS_ONE);
            for (int i = sind; i < dind; ++i)
                out.emitC(DEST_A | COMP_A_PLUS_ONE);
            out.emitC(DEST_M | COMP_D); // store
            return;
        }
    }

    // TODO: do not duplicate writePop()
    load(DEST_D, dseg, dind);
    out.emitA("R15");
    out.emitC(DEST_M | COMP_D); // save calculated address for popping
    push_load(sseg, sind);
    out.emitA("R15");
    out.emitC(DEST_A | COMP_M); // load the address
    out.emitC(DEST_M | COMP_D); // copy
}
/*
 * UNARY, BINARY
 */
void VMWriter::writeUnary(bool in, bool fin, UnaryOperation op, int intArg)
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
            out.emitA("SP");
            out.emitC(DEST_A | COMP_M_MINUS_ONE);
            out.emitC(unaryTable[op]);
            break;
        case DOUBLE:
            out.emitC(DEST_D | COMP_M);
            out.emitC(DEST_M | COMP_D_PLUS_M);
            break;
        case SUBC:
            if (intArg == 1) {
                out.emitA("SP");
                out.emitC(DEST_A | COMP_M_MINUS_ONE);
                out.emitC(DEST_M | COMP_M_MINUS_ONE);
                return;
            }
        // no break;
        case ADDC:
        case BUSC:
        case ANDC:
        case ORC:
            out.emitA(intArg);
            out.emitC(DEST_D | COMP_A);
            out.emitA("SP");
            out.emitC(DEST_A | COMP_M_MINUS_ONE);
            out.emitC(unaryTable[op]);
            break;
        }

    } else {
        if (in)
            pop();
        switch (op) {
        case NOT:
            out.emitC(DEST_D | COMP_NOT_D);
            break;
        case NEG:
            out.emitC(DEST_D | COMP_MINUS_D);
            break;
        case DOUBLE:
            out.emitC(DEST_A | COMP_D);
            out.emitC(DEST_D | COMP_D_PLUS_A);
            break;
        case ADDC:
            if (intArg == 1) {
                out.emitC(DEST_D | COMP_D_PLUS_ONE);
            } else {
                out.emitA(intArg);
                out.emitC(DEST_D | COMP_D_PLUS_A);
            }
            break;
        case SUBC:
            if (intArg == 1) {
                out.emitC(DEST_D | COMP_D_MINUS_ONE);
            } else {
                out.emitA(intArg);
                out.emitC(DEST_D | COMP_D_MINUS_A);
            }
            break;
        case BUSC:
            out.emitA(intArg);
            out.emitC(DEST_D | COMP_A_MINUS_D);
            break;
        case ANDC:
            out.emitA(intArg);
            out.emitC(DEST_D | COMP_D_AND_A);
            break;
        case ORC:
            out.emitA(intArg);
            out.emitC(DEST_D | COMP_D_OR_A);
            break;
        }
        if (fin)
            push();
    }
}
void VMWriter::writeBinary(bool in, bool fin, BinaryOperation op)
{
    unsigned short dest;
    if (fin) { // store result to memory; do not adjust SP
        dest = DEST_M;
        poptop(in);
    } else { // store result to register; adjust SP
        dest = DEST_D;
        if (in) // fetch argument from stack
            pop();
        out.emitA("SP");
        out.emitC(DEST_A | DEST_M | COMP_M_MINUS_ONE);
    }
    switch (op) {
    case ADD:
        out.emitC(dest | COMP_D_PLUS_M);
        break;
    case SUB:
        out.emitC(dest | COMP_M_MINUS_D);
        break;
    case BUS:
        out.emitC(dest | COMP_D_MINUS_M);
        break;
    case AND:
        out.emitC(dest | COMP_D_AND_M);
        break;
    case OR:
        out.emitC(dest | COMP_D_OR_M);
        break;
    }
}
/*
 * UNARY_COMPARE, COMPARE
 */
void VMWriter::compareBranches(bool fin, CompareOperation op)
{
    std::string compareSwitch = constructString("__compareSwitch", compareCounter);
    std::string compareEnd = constructString("__compareEnd", compareCounter);
    ++compareCounter;

    if (fin) {
        out.emitA(compareEnd);
        out.emitC(COMP_D | op.jump());
        out.emitA("SP");
        out.emitC(DEST_A | COMP_M_MINUS_ONE);
        out.emitC(DEST_M | COMP_ZERO); // adjust to false
        out.emitL(compareEnd);
    } else {
        out.emitA(compareSwitch);
        out.emitC(COMP_D | op.jump());
        out.emitC(DEST_D | COMP_ZERO);
        out.emitA(compareEnd);
        out.emitC(COMP_ZERO | JMP);
        out.emitL(compareSwitch);
        out.emitC(DEST_D | COMP_MINUS_ONE);
        out.emitL(compareEnd);
    }
}
void VMWriter::writeUnaryCompare(bool in, bool fin, CompareOperation op, int intArg)
{
    if (fin) { // save result to memory
        if (in) { // fetch argument from stack, do not adjust SP
            out.emitA("SP");
            out.emitC(DEST_A | COMP_M_MINUS_ONE);
            out.emitC(DEST_D | COMP_M);
        } else { // argument is in register, increment SP
            out.emitA("SP");
            out.emitC(DEST_M | COMP_M_PLUS_ONE);
            out.emitC(DEST_A | COMP_M_MINUS_ONE);
        }
        out.emitC(DEST_M | COMP_MINUS_ONE); // default is true
    } else { // save result to register
        if (in)
            pop(); // fetch argument from stack
    }
    unaryCompare(intArg);
    compareBranches(fin, op);
}
void VMWriter::writeCompare(bool in, bool fin, CompareOperation op)
{
    poptop(in);
    out.emitC(DEST_D | COMP_M_MINUS_D); // comparison
    if (fin) {
        out.emitC(DEST_M | COMP_MINUS_ONE); // default is true
    } else {
        out.emitA("SP");
        out.emitC(DEST_M | COMP_M_MINUS_ONE);
    }
    compareBranches(fin, op);
}
/*
 * LABEL, GOTO, IF, COMPARE_IF, UNARY_COMPARE_IF
 */
void VMWriter::writeLabel(const std::string label) { out.emitL(constructString(function, label)); }
void VMWriter::writeGoto(const std::string label)
{
    out.emitA(constructString(function, label));
    out.emitC(COMP_ZERO | JMP);
}
void VMWriter::writeIf(bool in, bool fin, CompareOperation op, const std::string label,
                       bool compare, bool useConst, int intConst)
{
    if (in)
        pop();
    if (compare) {
        if (useConst) { // UNARY_COMPARE_IF
            unaryCompare(intConst);
        } else { // COMPARE_IF
            out.emitA("SP");
            out.emitC(DEST_A | DEST_M | COMP_M_MINUS_ONE); // --SP
            out.emitC(DEST_D | COMP_M_MINUS_D); // comparison
        }
    } // else just IF
    out.emitA(constructString(function, label));
    out.emitC(COMP_D | op.jump());
}
/*
 * FUNCTION, CALL, RETURN
 */
void VMWriter::writeFunction(const std::string name, int localc)
{
    function = name;
    out.emitL(name);
    switch (localc) {
    case 0:
        break;
    case 1:
        out.emitA("SP");
        out.emitC(DEST_M | COMP_M_PLUS_ONE);
        out.emitC(DEST_A | COMP_M_MINUS_ONE);
        out.emitC(DEST_M | COMP_ZERO);
        break;
    default: // 2*localc+4 instructions
        out.emitA("SP");
        out.emitC(DEST_A | COMP_M);
        for (int i = 1; i < localc; ++i) {
            out.emitC(DEST_M | COMP_ZERO);
            out.emitC(DEST_A | COMP_A_PLUS_ONE);
        }
        out.emitC(DEST_M | COMP_ZERO);
        out.emitC(DEST_D | COMP_A_PLUS_ONE); // "unroll"
        out.emitA("SP");
        out.emitC(DEST_M | COMP_D);
        break;
    }
}
// 44 instructions when called for the first time
//  8 instructions when called next time with the same argc
void VMWriter::writeCall(const std::string name, int argc)
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

    out.emitA(name);
    out.emitC(DEST_D | COMP_A);
    out.emitA("R15");
    out.emitC(DEST_M | COMP_D);
    out.emitA(returnAddress);
    out.emitC(DEST_D | COMP_A);
    if (found) {
        out.emitA(call);
        out.emitC(COMP_ZERO | JMP);
    } else {
        out.emitL(call);
        push();
        out.emitA("LCL");
        out.emitC(DEST_D | COMP_M);
        push();
        out.emitA("ARG");
        out.emitC(DEST_D | COMP_M);
        push();
        out.emitA("THIS");
        out.emitC(DEST_D | COMP_M);
        push();
        out.emitA("THAT");
        out.emitC(DEST_D | COMP_M);
        push();
        out.emitA("SP");
        out.emitC(DEST_D | COMP_M);
        out.emitA("LCL");
        out.emitC(DEST_M | COMP_D); // LCL = SP
        out.emitA(argc + 5);
        out.emitC(DEST_D | COMP_D_MINUS_A);
        out.emitA("ARG");
        out.emitC(DEST_M | COMP_D); // ARG = SP - " << argc << " - 5\n"
        out.emitA("R15");
        out.emitC(DEST_A | COMP_M | JMP);
        argStubs.push_back(argc);
    }
    out.emitL(returnAddress);
}

void VMWriter::writeReturn()
{
    out.emitA("__return");
    out.emitC(COMP_ZERO | JMP);
}
/*
 * BOOTSTRAP
 */
void VMWriter::writeBootstrap()
{
    out.emitA(256);
    out.emitC(DEST_D | COMP_A);
    out.emitA("SP");
    out.emitC(DEST_M | COMP_D);
    writeCall("Sys.init", 0);
    out.emitL("__return");
    out.emitA(5);
    out.emitC(DEST_D | COMP_A);
    out.emitA("LCL");
    out.emitC(DEST_A | COMP_M_MINUS_D);
    out.emitC(DEST_D | COMP_M);
    out.emitA("R15");
    out.emitC(DEST_M | COMP_D); // R15 = *(LCL-5)
    out.emitA("SP");
    out.emitC(DEST_A | DEST_M | COMP_M_MINUS_ONE);
    out.emitC(DEST_D | COMP_M); // return value
    out.emitA("ARG");
    out.emitC(DEST_A | COMP_M);
    out.emitC(DEST_M | COMP_D); // *ARG = pop()
    out.emitC(DEST_D | COMP_A_PLUS_ONE);
    out.emitA("SP");
    out.emitC(DEST_M | COMP_D); // SP = ARG + 1
    out.emitA("LCL");
    out.emitC(DEST_D | COMP_M);
    out.emitA("R14");
    out.emitC(DEST_A | DEST_M | COMP_D_MINUS_ONE); // R14 = LCL - 1
    out.emitC(DEST_D | COMP_M);
    out.emitA("THAT");
    out.emitC(DEST_M | COMP_D); // THAT = M[R14]
    out.emitA("R14");
    out.emitC(DEST_A | DEST_M | COMP_M_MINUS_ONE);
    out.emitC(DEST_D | COMP_M);
    out.emitA("THIS");
    out.emitC(DEST_M | COMP_D); // THIS = M[R14--]
    out.emitA("R14");
    out.emitC(DEST_A | DEST_M | COMP_M_MINUS_ONE);
    out.emitC(DEST_D | COMP_M);
    out.emitA("ARG");
    out.emitC(DEST_M | COMP_D); // ARG = M[R14--]
    out.emitA("R14");
    out.emitC(DEST_A | DEST_M | COMP_M_MINUS_ONE);
    out.emitC(DEST_D | COMP_M);
    out.emitA("LCL");
    out.emitC(DEST_M | COMP_D); // LCL = M[R14--]
    out.emitA("R15");
    out.emitC(DEST_A | COMP_M | JMP); // goto R15
}

void VMWriter::write(const VMCommand& c)
{
    {
        std::stringstream ss;
        ss << c;
        out.emitComment(ss.str());
    }
    switch (c.type) {
    case VMCommand::CONSTANT:
        writeConstant(c.in, c.fin, c.int1);
        break;
    case VMCommand::PUSH:
        writePush(c.in, c.fin, c.segment1, c.int1);
        break;
    case VMCommand::POP_DIRECT:
        writePopDirect(c.in, c.fin, c.segment1, c.int1);
        break;
    case VMCommand::POP_INDIRECT:
        writePopIndirect(c.segment1, c.int1);
        break;
    case VMCommand::POP_INDIRECT_PUSH:
        writePopIndirectPush(c.in, c.fin, c.segment1, c.int1);
        break;
    case VMCommand::COPY:
        writeCopy(c.segment1, c.int1, c.segment2, c.int2);
        break;
    case VMCommand::UNARY:
        writeUnary(c.in, c.fin, c.unary, c.int1);
        break;
    case VMCommand::BINARY:
        writeBinary(c.in, c.fin, c.binary);
        break;
    case VMCommand::COMPARE:
        writeCompare(c.in, c.fin, c.compare);
        break;
    case VMCommand::UNARY_COMPARE:
        writeUnaryCompare(c.in, c.fin, c.compare, c.int1);
        break;
    case VMCommand::LABEL:
        writeLabel(c.arg1);
        break;
    case VMCommand::GOTO:
        writeGoto(c.arg1);
        break;
    case VMCommand::IF:
        writeIf(c.in, c.fin, c.compare, c.arg1, false, false, 0);
        break;
    case VMCommand::COMPARE_IF:
        writeIf(c.in, c.fin, c.compare, c.arg1, true, false, 0);
        break;
    case VMCommand::UNARY_COMPARE_IF:
        writeIf(c.in, c.fin, c.compare, c.arg1, true, true, c.int1);
        break;
    case VMCommand::FUNCTION:
        writeFunction(c.arg1, c.int1);
        break;
    case VMCommand::CALL:
        writeCall(c.arg1, c.int1);
        break;
    case VMCommand::RETURN:
        writeReturn();
        break;
    case VMCommand::NOP:
    case VMCommand::IN:
    case VMCommand::FIN:
        throw std::runtime_error("helper commands made it to the final version");
        break;
    }
}

} // namespace hcc {
