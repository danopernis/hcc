// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/assembler/asm.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <stdexcept>
#include <unordered_map>

namespace hcc {
namespace assembler {

namespace {

const std::map<std::string, unsigned short> destMap = {
    {"M", hcc::instruction::DEST_M},
    {"D", hcc::instruction::DEST_D},
    {"MD", hcc::instruction::DEST_M | hcc::instruction::DEST_D},
    {"A", hcc::instruction::DEST_A},
    {"AM", hcc::instruction::DEST_A | hcc::instruction::DEST_M},
    {"AD", hcc::instruction::DEST_A | hcc::instruction::DEST_D},
    {"AMD", hcc::instruction::DEST_A | hcc::instruction::DEST_M | hcc::instruction::DEST_D},
};

const std::map<std::string, unsigned short> jumpMap = {
    {"JGT", hcc::instruction::JGT},
    {"JEQ", hcc::instruction::JEQ},
    {"JGE", hcc::instruction::JGE},
    {"JLT", hcc::instruction::JLT},
    {"JNE", hcc::instruction::JNE},
    {"JLE", hcc::instruction::JLE},
    {"JMP", hcc::instruction::JMP},
};

const std::map<std::string, unsigned short> compMap = {
    {"0", hcc::instruction::COMP_ZERO},
    {"1", hcc::instruction::COMP_ONE},
    {"!D", hcc::instruction::COMP_NOT_D},
    {"!A", hcc::instruction::COMP_NOT_A},
    {"!M", hcc::instruction::COMP_NOT_M},
    {"-1", hcc::instruction::COMP_MINUS_ONE},
    {"-D", hcc::instruction::COMP_MINUS_D},
    {"-A", hcc::instruction::COMP_MINUS_A},
    {"-M", hcc::instruction::COMP_MINUS_M},
    {"M", hcc::instruction::COMP_M},
    {"M+1", hcc::instruction::COMP_M_PLUS_ONE},
    {"M-1", hcc::instruction::COMP_M_MINUS_ONE},
    {"M-D", hcc::instruction::COMP_M_MINUS_D},
    {"A", hcc::instruction::COMP_A},
    {"A+1", hcc::instruction::COMP_A_PLUS_ONE},
    {"A-1", hcc::instruction::COMP_A_MINUS_ONE},
    {"A-D", hcc::instruction::COMP_A_MINUS_D},
    {"D", hcc::instruction::COMP_D},
    {"D+1", hcc::instruction::COMP_D_PLUS_ONE},
    {"D+A", hcc::instruction::COMP_D_PLUS_A},
    {"D+M", hcc::instruction::COMP_D_PLUS_M},
    {"D-1", hcc::instruction::COMP_D_MINUS_ONE},
    {"D-A", hcc::instruction::COMP_D_MINUS_A},
    {"D-M", hcc::instruction::COMP_D_MINUS_M},
    {"D&A", hcc::instruction::COMP_D_AND_A},
    {"D&M", hcc::instruction::COMP_D_AND_M},
    {"D|A", hcc::instruction::COMP_D_OR_A},
    {"D|M", hcc::instruction::COMP_D_OR_M},
};

void instructionToString(std::ostream& out, unsigned short instr)
{
    if (instr & hcc::instruction::COMPUTE) {
        if (instr & hcc::instruction::MASK_DEST) {
            if (instr & hcc::instruction::DEST_A) {
                out << 'A';
            }
            if (instr & hcc::instruction::DEST_M) {
                out << 'M';
            }
            if (instr & hcc::instruction::DEST_D) {
                out << 'D';
            }
            out << '=';
        }

        switch (instr & hcc::instruction::MASK_COMP) {
        case hcc::instruction::COMP_ZERO:
            out << "0";
            break;
        case hcc::instruction::COMP_ONE:
            out << "1";
            break;
        case hcc::instruction::COMP_MINUS_ONE:
            out << "-1";
            break;
        case hcc::instruction::COMP_D:
            out << "D";
            break;
        case hcc::instruction::COMP_A:
            out << "A";
            break;
        case hcc::instruction::COMP_NOT_D:
            out << "!D";
            break;
        case hcc::instruction::COMP_NOT_A:
            out << "!A";
            break;
        case hcc::instruction::COMP_MINUS_D:
            out << "-D";
            break;
        case hcc::instruction::COMP_MINUS_A:
            out << "-A";
            break;
        case hcc::instruction::COMP_D_PLUS_ONE:
            out << "D+1";
            break;
        case hcc::instruction::COMP_A_PLUS_ONE:
            out << "A+1";
            break;
        case hcc::instruction::COMP_D_MINUS_ONE:
            out << "D-1";
            break;
        case hcc::instruction::COMP_A_MINUS_ONE:
            out << "A-1";
            break;
        case hcc::instruction::COMP_D_PLUS_A:
            out << "D+A";
            break;
        case hcc::instruction::COMP_D_MINUS_A:
            out << "D-A";
            break;
        case hcc::instruction::COMP_A_MINUS_D:
            out << "A-D";
            break;
        case hcc::instruction::COMP_D_AND_A:
            out << "D&A";
            break;
        case hcc::instruction::COMP_D_OR_A:
            out << "D|A";
            break;
        case hcc::instruction::COMP_M:
            out << "M";
            break;
        case hcc::instruction::COMP_NOT_M:
            out << "!M";
            break;
        case hcc::instruction::COMP_MINUS_M:
            out << "-M";
            break;
        case hcc::instruction::COMP_M_PLUS_ONE:
            out << "M+1";
            break;
        case hcc::instruction::COMP_M_MINUS_ONE:
            out << "M-1";
            break;
        case hcc::instruction::COMP_D_PLUS_M:
            out << "D+M";
            break;
        case hcc::instruction::COMP_D_MINUS_M:
            out << "D-M";
            break;
        case hcc::instruction::COMP_M_MINUS_D:
            out << "M-D";
            break;
        case hcc::instruction::COMP_D_AND_M:
            out << "D&M";
            break;
        case hcc::instruction::COMP_D_OR_M:
            out << "D|M";
            break;
        default:
            throw std::runtime_error("C-instruction uses undocumented computation");
        }

        switch (instr & hcc::instruction::MASK_JUMP) {
        case hcc::instruction::JLT:
            out << ";JLT";
            break;
        case hcc::instruction::JGT:
            out << ";JGT";
            break;
        case hcc::instruction::JLE:
            out << ";JLE";
            break;
        case hcc::instruction::JGE:
            out << ";JGE";
            break;
        case hcc::instruction::JNE:
            out << ";JNE";
            break;
        case hcc::instruction::JEQ:
            out << ";JEQ";
            break;
        case hcc::instruction::JMP:
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

} // namespace {

void program::emitLoadSymbolic(std::string symbol)
{
    instruction i;
    i.type = instruction_type::LOAD;
    i.symbol = std::move(symbol);
    instructions.push_back(std::move(i));
}
void program::emitLoadConstant(cpu::word constant)
{
    instruction i;
    i.type = instruction_type::VERBATIM;

    if (constant & hcc::instruction::COMPUTE) {
        i.instr = std::abs((signed short)constant);
        instructions.push_back(std::move(i));
        emitInstruction(hcc::instruction::DEST_A | hcc::instruction::COMP_MINUS_A);
    } else {
        i.instr = constant;
        instructions.push_back(std::move(i));
    }
}
void program::emitInstruction(cpu::word instruction_)
{
    instruction i;
    i.type = instruction_type::VERBATIM;
    i.instr = instruction_ | hcc::instruction::RESERVED | hcc::instruction::COMPUTE;
    instructions.push_back(std::move(i));
}
void program::emitLabel(std::string label)
{
    instruction i;
    i.type = instruction_type::LABEL;
    i.symbol = std::move(label);
    instructions.push_back(std::move(i));
}
void program::emitComment(std::string comment)
{
    instruction i;
    i.type = instruction_type::COMMENT;
    i.symbol = std::move(comment);
    instructions.push_back(std::move(i));
}

std::vector<uint16_t> program::assemble() const
{
    // built-in symbols
    std::unordered_map<std::string, int> table = {
        {"SP", 0x0000},
        {"LCL", 0x0001},
        {"ARG", 0x0002},
        {"THIS", 0x0003},
        {"THAT", 0x0004},

        {"SCREEN", 0x4000},
        {"KBD", 0x6000},

        {"R0", 0x0000},
        {"R1", 0x0001},
        {"R2", 0x0002},
        {"R3", 0x0003},
        {"R4", 0x0004},
        {"R5", 0x0005},
        {"R6", 0x0006},
        {"R7", 0x0007},
        {"R8", 0x0008},
        {"R9", 0x0009},
        {"R10", 0x000a},
        {"R11", 0x000b},
        {"R12", 0x000c},
        {"R13", 0x000d},
        {"R14", 0x000e},
        {"R15", 0x000f},
    };

    // first pass
    int address = 0;
    for (const auto& c : instructions) {
        switch (c.type) {
        case instruction_type::LABEL: {
            // assign address to label
            const auto x = table.emplace(c.symbol, address);
            if (!x.second) {
                throw std::runtime_error{"Duplicate label " + c.symbol};
            }
            break; }
        case instruction_type::LOAD:
        case instruction_type::VERBATIM:
            // maintain address
            ++address;
            break;
        case instruction_type::COMMENT:
            break;
        }
    }

    // second pass
    int variable = 0x10;
    std::vector<uint16_t> result;
    result.reserve(address);
    for (const auto& c : instructions) {
        switch (c.type) {
        case instruction_type::LABEL:
        case instruction_type::COMMENT:
            // ignore
            break;
        case instruction_type::LOAD: {
            const auto x = table.emplace(c.symbol, variable);
            if (x.second) {
                ++variable;
            }
            result.push_back(x.first->second);
            break; }
        case instruction_type::VERBATIM:
            result.push_back(c.instr);
            break;
        }
    }
    return result;
}

program::program(std::istream& input)
{
    std::string line;
    while (std::getline(input, line)) {
        // ignore blank lines
        if (line.empty()) {
            continue;
        }

        instruction i;
        if (line.find("//") == 0) {
            i.type = instruction_type::COMMENT;
            i.symbol = line.substr(2, line.length());
        } else if (line.at(0) == '@') {
            std::string symbol = line.substr(1, line.length() - 1);
            if (isdigit(symbol.at(0))) {
                i.type = instruction_type::VERBATIM;
                std::stringstream ss(symbol);
                ss >> i.instr;
            } else {
                i.type = instruction_type::LOAD;
                i.symbol = symbol;
            }
        } else if (line.at(0) == '(') {
            i.type = instruction_type::LABEL;
            i.symbol = line.substr(1, line.length() - 2);
        } else {
            i.type = instruction_type::VERBATIM;
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
                comp = compMap.at(line.substr(equals + 1, length - equals - 1));
                jump = 0;
            } else {
                comp = compMap.at(line.substr(equals + 1, semicolon - equals - 1));
                jump = jumpMap.at(line.substr(semicolon + 1, 3));
            }

            i.instr = hcc::instruction::COMPUTE | hcc::instruction::RESERVED | comp | dest | jump;
        }

        instructions.push_back(std::move(i));
    }
}

void program::save(const std::string& filename) const
{
    std::ofstream out(filename);
    for (const auto& i : instructions) {
        switch (i.type) {
        case instruction_type::LOAD:
            out << '@' << i.symbol << '\n';
            break;
        case instruction_type::VERBATIM:
            instructionToString(out, i.instr);
            out << '\n';
            break;
        case instruction_type::LABEL:
            out << '(' << i.symbol << ")\n";
            break;
        case instruction_type::COMMENT:
            out << "//" << i.symbol << "\n";
            break;
        }
    }
}

void saveHACK(const std::string& filename, std::vector<uint16_t> instructions)
{
    char line[17];
    line[16] = '\n';
    std::ofstream out(filename.c_str());
    for (auto instr : instructions) {
        for (int j = 0; j < 16; ++j) {
            line[j] = (instr & (1 << 15)) ? '1' : '0';
            instr <<= 1;
        }
        out.write(line, 17);
    }
}

} // namespace assembler {
} // namespace hcc {
