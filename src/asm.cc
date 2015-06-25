// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <stdexcept>
#include "asm.h"
#include "instruction.h"

namespace {

using namespace hcc::instruction;

const std::map<std::string, unsigned short> destMap = {
    { "M",      DEST_M },
    { "D",      DEST_D },
    { "MD",     DEST_M | DEST_D },
    { "A",      DEST_A },
    { "AM",     DEST_A | DEST_M },
    { "AD",     DEST_A | DEST_D },
    { "AMD",    DEST_A | DEST_M | DEST_D },
};

const std::map<std::string, unsigned short> jumpMap = {
    { "JGT",    JGT },
    { "JEQ",    JEQ },
    { "JGE",    JGE },
    { "JLT",    JLT },
    { "JNE",    JNE },
    { "JLE",    JLE },
    { "JMP",    JMP },
};

const std::map<std::string, unsigned short> compMap = {
    { "0",      COMP_ZERO },
    { "1",      COMP_ONE },
    { "!D",     COMP_NOT_D },
    { "!A",     COMP_NOT_A },
    { "!M",     COMP_NOT_M },
    { "-1",     COMP_MINUS_ONE },
    { "-D",     COMP_MINUS_D },
    { "-A",     COMP_MINUS_A },
    { "-M",     COMP_MINUS_M },
    { "M",      COMP_M },
    { "M+1",    COMP_M_PLUS_ONE },
    { "M-1",    COMP_M_MINUS_ONE },
    { "M-D",    COMP_M_MINUS_D },
    { "A",      COMP_A },
    { "A+1",    COMP_A_PLUS_ONE },
    { "A-1",    COMP_A_MINUS_ONE },
    { "A-D",    COMP_A_MINUS_D },
    { "D",      COMP_D },
    { "D+1",    COMP_D_PLUS_ONE },
    { "D+A",    COMP_D_PLUS_A },
    { "D+M",    COMP_D_PLUS_M },
    { "D-1",    COMP_D_MINUS_ONE },
    { "D-A",    COMP_D_MINUS_A },
    { "D-M",    COMP_D_MINUS_M },
    { "D&A",    COMP_D_AND_A },
    { "D&M",    COMP_D_AND_M },
    { "D|A",    COMP_D_OR_A },
    { "D|M",    COMP_D_OR_M },
};

void instructionToString(
    std::ostream& out,
    unsigned short instr)
{
    if (instr & COMPUTE) {
        if (instr & MASK_DEST) {
            if (instr & DEST_A) {
                out << 'A';
            }
            if (instr & DEST_M) {
                out << 'M';
            }
            if (instr & DEST_D) {
                out << 'D';
            }
            out << '=';
        }

        switch (instr & MASK_COMP) {
        case COMP_ZERO:
            out << "0";
            break;
        case COMP_ONE:
            out << "1";
            break;
        case COMP_MINUS_ONE:
            out << "-1";
            break;
        case COMP_D:
            out << "D";
            break;
        case COMP_A:
            out << "A";
            break;
        case COMP_NOT_D:
            out << "!D";
            break;
        case COMP_NOT_A:
            out << "!A";
            break;
        case COMP_MINUS_D:
            out << "-D";
            break;
        case COMP_MINUS_A:
            out << "-A";
            break;
        case COMP_D_PLUS_ONE:
            out << "D+1";
            break;
        case COMP_A_PLUS_ONE:
            out << "A+1";
            break;
        case COMP_D_MINUS_ONE:
            out << "D-1";
            break;
        case COMP_A_MINUS_ONE:
            out << "A-1";
            break;
        case COMP_D_PLUS_A:
            out << "D+A";
            break;
        case COMP_D_MINUS_A:
            out << "D-A";
            break;
        case COMP_A_MINUS_D:
            out << "A-D";
            break;
        case COMP_D_AND_A:
            out << "D&A";
            break;
        case COMP_D_OR_A:
            out << "D|A";
            break;
        case COMP_M:
            out << "M";
            break;
        case COMP_NOT_M:
            out << "!M";
            break;
        case COMP_MINUS_M:
            out << "-M";
            break;
        case COMP_M_PLUS_ONE:
            out << "M+1";
            break;
        case COMP_M_MINUS_ONE:
            out << "M-1";
            break;
        case COMP_D_PLUS_M:
            out << "D+M";
            break;
        case COMP_D_MINUS_M:
            out << "D-M";
            break;
        case COMP_M_MINUS_D:
            out << "M-D";
            break;
        case COMP_D_AND_M:
            out << "D&M";
            break;
        case COMP_D_OR_M:
            out << "D|M";
            break;
        default:
            throw std::runtime_error("C-instruction uses undocumented computation");
        }

        switch (instr & MASK_JUMP) {
        case JLT:
            out << ";JLT";
            break;
        case JGT:
            out << ";JGT";
            break;
        case JLE:
            out << ";JLE";
            break;
        case JGE:
            out << ";JGE";
            break;
        case JNE:
            out << ";JNE";
            break;
        case JEQ:
            out << ";JEQ";
            break;
        case JMP:
            out << ";JMP";
            break;
        default:
            /* do not jump */
            break;
        }
    } else {
        out << '@' << instr;
    }
}

} // anonymous namespace

namespace hcc {

std::vector<uint16_t> asm_program::assemble() const
{
    // built-in symbols
    std::map<std::string, int> table = {
        { "SP",     0x0000 },
        { "LCL",    0x0001 },
        { "ARG",    0x0002 },
        { "THIS",   0x0003 },
        { "THAT",   0x0004 },

        { "SCREEN", 0x4000 },
        { "KBD",    0x6000 },

        { "R0",     0x0000 },
        { "R1",     0x0001 },
        { "R2",     0x0002 },
        { "R3",     0x0003 },
        { "R4",     0x0004 },
        { "R5",     0x0005 },
        { "R6",     0x0006 },
        { "R7",     0x0007 },
        { "R8",     0x0008 },
        { "R9",     0x0009 },
        { "R10",    0x000a },
        { "R11",    0x000b },
        { "R12",    0x000c },
        { "R13",    0x000d },
        { "R14",    0x000e },
        { "R15",    0x000f },
    };

    // first pass
    int address = 0;
    for (auto& c : instructions) {
        switch (c.type) {
        case asm_instruction_type::LABEL:
            // assign address to label
            table[c.symbol] = address;
            break;
        case asm_instruction_type::LOAD:
        case asm_instruction_type::VERBATIM:
            // maintain address
            ++address;
            break;
        case asm_instruction_type::COMMENT:
            break;
        }
    }

    // second pass
    int variable = 0x10;
    std::vector<uint16_t> result;
    for (auto& c : instructions) {
        switch (c.type) {
        case asm_instruction_type::LABEL:
        case asm_instruction_type::COMMENT:
            // ignore
            break;
        case asm_instruction_type::LOAD:
            if (table.find(c.symbol) == table.end()) {
                table[c.symbol] = variable++;
            }
            result.push_back(table[c.symbol]);
            break;
        case asm_instruction_type::VERBATIM:
            result.push_back(c.instr);
            break;
        }
    }
    return result;
}

asm_program::asm_program(std::istream& input)
{
    std::string line;
    while (std::getline(input, line)) {
        // ignore blank lines
        if (line.empty()) {
            continue;
        }

        asm_instruction i;
        if (line.find("//") == 0) {
            i.type = asm_instruction_type::COMMENT;
            i.symbol = line.substr(2, line.length());
        } else if (line.at(0) == '@') {
            std::string symbol = line.substr(1, line.length()-1);
            if (isdigit(symbol.at(0))) {
                i.type = asm_instruction_type::VERBATIM;
                std::stringstream ss(symbol);
                ss >> i.instr;
            } else {
                i.type = asm_instruction_type::LOAD;
                i.symbol = symbol;
            }
        } else if (line.at(0) == '(') {
            i.type = asm_instruction_type::LABEL;
            i.symbol = line.substr(1, line.length()-2);
        } else {
            i.type = asm_instruction_type::VERBATIM;
            // dest=comp;jump
            auto length = line.length();
            auto equals = line.find('=');
            auto semicolon = line.rfind(';');

            unsigned short dest, comp, jump;

            if (equals > length || equals == 0) {
                dest = 0;
                equals = -1;
            } else {
                dest = destMap.at(line.substr(0, equals));
            }
            if (semicolon > length) {
                comp = compMap.at(line.substr(equals+1, length-equals-1));
                jump = 0;
            } else {
                comp = compMap.at(line.substr(equals+1, semicolon-equals-1));
                jump = jumpMap.at(line.substr(semicolon+1, 3));
            }

            i.instr = instruction::COMPUTE | instruction::RESERVED | comp | dest | jump;
        }

        instructions.push_back(std::move(i));
    }
}

void asm_program::save(const std::string& filename) const
{
    std::ofstream out(filename);
    for (const auto& i : instructions) {
        switch (i.type) {
        case asm_instruction_type::LOAD:
            out << '@' << i.symbol << '\n';
            break;
        case asm_instruction_type::VERBATIM:
            instructionToString(out, i.instr);
            out << '\n';
            break;
        case asm_instruction_type::LABEL:
            out << '(' << i.symbol << ")\n";
            break;
        case asm_instruction_type::COMMENT:
            out << "//" << i.symbol << "\n";
            break;
        }
    }
}

void saveHACK(const std::string& filename, std::vector<uint16_t> instructions)
{
    std::ofstream out(filename.c_str());
    for (auto instr : instructions) {
        for (int j = 0; j<16; ++j) {
            out << (instr & (1<<15) ? '1' : '0');
            instr <<= 1;
        }
        out << '\n';
    }
}

} //end namespace
