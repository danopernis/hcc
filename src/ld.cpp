/*
 * Copyright (c) 2013 Dano Pernis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <vector>
#include <cassert>
#include "instruction.h"

namespace Exception {

/**
 * An exception tracing filename and line number
 */
class ParserError
    : public std::exception
{
public:
    ParserError(const std::string& fileName,
                   unsigned lineNumber)
        : fileName(fileName)
        , lineNumber(lineNumber)
    {}

    const std::string& getFileName() const
    {
        return fileName;
    }

    const unsigned getLineNumber() const
    {
        return lineNumber;
    }

private:
    std::string fileName;
    unsigned lineNumber;
};

/**
 * Thrown when previously defined symbol is encountered.
 */
class SymbolAlreadyDefined
    : public ParserError
{
public:
    SymbolAlreadyDefined(const std::string& fileName,
                         unsigned lineNumber)
        : ParserError(fileName, lineNumber)
    {}
};

/**
 * Thrown when new subroutine is started without ending the previously started
 * subroutine.
 */
class NestedSubroutine
    : public ParserError
{
public:
    NestedSubroutine(const std::string& fileName,
                     unsigned lineNumber)
        : ParserError(fileName, lineNumber)
    {}
};

/**
 * Thrown when instruction occurs outside of a subroutine.
 */
class InstructionLeak
    : public ParserError
{
public:
    InstructionLeak(const std::string& fileName,
                    unsigned lineNumber)
        : ParserError(fileName, lineNumber)
    {}
};

/**
 * Thrown when relocation occurs outside of a subroutine.
 */
class RelocationLeak
    : public ParserError
{
public:
    RelocationLeak(const std::string& fileName,
                   unsigned lineNumber)
        : ParserError(fileName, lineNumber)
    {}
};

/**
 * Thrown when subroutine ends before it started.
 */
class SubroutineEndBeforeStart
    : public ParserError
{
public:
    SubroutineEndBeforeStart(const std::string& fileName,
                             unsigned lineNumber)
        : ParserError(fileName, lineNumber)
    {}
};

/**
 * Thrown when subroutine is started, but does not end.
 */
class SubroutineNotEnded
    : public ParserError
{
public:
    SubroutineNotEnded(const std::string& fileName,
                       unsigned lineNumber)
        : ParserError(fileName, lineNumber)
    {}
};

} // end namespace

/*
 * Data structures
 */

typedef std::map<std::string, unsigned> AddressTable;

/**
 * Instruction represents either "verbatim" or "relocated" instruction.
 */
class Instruction {
public:
    /**
     * Creates a "verbatim" instruction. Verbatim instruction is not translated
     * during relocation.
     *
     * @param raw is a raw instruction
     *
     * @return corresponding instruction
     */
    static Instruction createVerbatim(unsigned raw)
    {
        Instruction instruction;
        instruction.relocate = false;
        instruction.value = raw;
        return instruction;
    }

    /**
     * Creates a "relocated" instruction. Relocated instruction is translated
     * during relocation.
     *
     * @param symbol is name of the symbol used in relocation
     * @param offset is offset used in relocation
     *
     * @return corresponding instruction
     */
    static Instruction createRelocated(const std::string& symbol, unsigned offset)
    {
        Instruction instruction;
        instruction.relocate = true;
        instruction.symbol = symbol;
        instruction.value = offset;
        return instruction;
    }

    /**
     * Translate instruction. Verbatim instruction is not translated, relocated
     * instruction is translated to its address. Address is computed as a sum
     * of base and offset. Offset is specified when the instruction is created.
     * Base is looked up in the provided address table.
     *
     * @param addressTable contains mapping between symbol name and its address
     *
     * @return raw instruction
     */
    unsigned translate(const AddressTable& addressTable) const
    {
        if (relocate) {
            unsigned base = addressTable.find(symbol)->second;
            return base + value;
        } else {
            return value;
        }
    }

private:
    Instruction() {}
    bool relocate;
    unsigned value;
    std::string symbol;
};

struct Subroutine {
    std::string symbol;
    std::set<std::string> dependencies;
    std::vector<Instruction> instructions;

    Subroutine(const std::string& symbol)
        : symbol(symbol)
    {}

    void write(std::ostream& output, AddressTable& addressTable) const
    {
        for (const auto& instruction: instructions) {
            hcc::instruction::outputBinary(output, instruction.translate(addressTable));
        }
    }
};

struct StaticVariable {
    std::string symbol;
    std::vector<unsigned> data;

    StaticVariable(const std::string& symbol,
                   const std::vector<unsigned>& data)
        : symbol(symbol)
        , data(data)
    {}
};

typedef std::map<std::string, Subroutine> SubroutinesMap;
typedef std::map<std::string, StaticVariable> StaticVariablesMap;

/*
 * Line parser, used in both scan and production pass.
 */

struct LineParser {
    enum LineType {INSTRUCTION, STATIC, SUBROUTINE_BEGIN, SUBROUTINE_END, RELOCATION, BLANK};

    unsigned instruction; // for LineType::INSTRUCTION
    unsigned offset; // for LineType::RELOCATION
    std::string symbol; // for LineType::STATIC, LineType::SUBROUTINE, and LineType::RELOCATION
    std::vector<unsigned> staticData; // for LineType::STATIC

    LineType parse(const std::string& line);
};

LineParser::LineType LineParser::parse(const std::string& line)
{
    if (line.size() == 0)
        return BLANK;

    if (line[0] == '!') {
        std::stringstream lineStream(line);

        // extract the command
        std::string command;
        lineStream >> command;

        // extract the first argument
        std::string argument;
        lineStream >> argument;

        // ...and interpret it
        if (command.compare("!static") == 0) {
            symbol = argument;

            // symbol is mandatory
            if (symbol.size() == 0)
                throw std::runtime_error("missing symbol name");

            // extract optional data
            staticData.clear();
            while (!lineStream.eof()) {
                std::string s;
                lineStream >> s;
                staticData.push_back(std::atoi(s.c_str()));
            }

            return STATIC;
        } else if (command.compare("!subroutine-begin") == 0) {
            symbol = argument;

            // symbol is mandatory
            if (symbol.size() == 0)
                throw std::runtime_error("missing symbol name");

            return SUBROUTINE_BEGIN;
        } else if (command.compare("!subroutine-end") == 0) {
            return SUBROUTINE_END;
        } else if (command.compare("!relocation") == 0) {
            // extract symbol
            auto plus = argument.find('+');
            symbol = argument.substr(0, plus);

            // extract offset
            if (plus == std::string::npos) {
                offset = 0; // default value when there is no '+'
            } else {
                if (plus+1<argument.size()) {
                    offset = std::atoi(argument.substr(plus+1, argument.size()).c_str());
                } else {
                    throw std::runtime_error("missing offset");
                }
            }

            return RELOCATION;
        } else {
            throw std::runtime_error("command is not supported");
        }
    }

    // it is not a command, so it must be 16 bit binary instruction
    instruction = 0;
    for (unsigned i = 0; i<line.size(); ++i) {
        char c = line[i];

        instruction <<= 1;
        if (c == '1') {
            instruction |= 1;
        } else if (c == '0') {
            // do nothing
        } else {
            break; // jump out and throw the error
        }

        // success if the string has 16 characters and each one was read
        if (line.size() == 16 && i == 15) {
            return INSTRUCTION;
        }
    }
    throw std::runtime_error("malformed instruction");
}

/**
 * Parse given file and collect information about all the static variables and
 * subroutines it defines.
 *
 * @param fileName   is a name of the input file
 * @param definedSymbols  is a set of all the defined symbols, whether it be
 *                        a subroutine or a static variable
 * @param subroutines     contains information about subroutines indexed by
 *                        symbol
 * @param staticVariables contains information about static variables indexed
 *                        by symbol
 */
void parse(const std::string& fileName,
           std::set<std::string>& definedSymbols,
           SubroutinesMap& subroutines,
           StaticVariablesMap& staticVariables)
try {
    // attempt to open the file
    std::ifstream inputFile(fileName.c_str());
    if (inputFile.fail())
        throw std::runtime_error("could not read file: " + fileName);

    // attempt to read the header
    std::string header;
    getline(inputFile, header);
    if (header.compare("!file object") != 0)
        throw std::runtime_error("not an object file: " + fileName);

    unsigned lineNumber = 1; // current line number for better error messages

    // actual work begins
    SubroutinesMap::iterator subroutine; // current subroutine
    bool inSubroutine = false;
    bool relocate = false; // indicates that next instruction is to be relocated

    while (inputFile.good()) {
        // read the input stream line by line
        std::string line;
        getline(inputFile, line);
        ++lineNumber;

        // parse line
        LineParser parser;
        auto type = parser.parse(line);

        // bail out if symbol is already defined
        switch (type) {
        case LineParser::STATIC:
        case LineParser::SUBROUTINE_BEGIN:
            if (definedSymbols.find(parser.symbol) == definedSymbols.end()) {
                definedSymbols.insert(parser.symbol);
            } else {
                throw Exception::SymbolAlreadyDefined(fileName, lineNumber);
            }
            break;
        case LineParser::INSTRUCTION:
        case LineParser::SUBROUTINE_END:
        case LineParser::RELOCATION:
        case LineParser::BLANK:
            // do nothing
            break;
        }

        switch (type) {
        case LineParser::STATIC:
            staticVariables.insert(std::make_pair<>(
                parser.symbol,
                StaticVariable(parser.symbol, parser.staticData)));
            break;

        case LineParser::SUBROUTINE_BEGIN:
            if (inSubroutine)
                throw Exception::NestedSubroutine(fileName, lineNumber);

            subroutine = subroutines.insert(
                std::make_pair<>(parser.symbol,
                                 Subroutine(parser.symbol))).first;

            inSubroutine = true;
            break;

        case LineParser::SUBROUTINE_END:
            if (!inSubroutine)
                throw Exception::SubroutineEndBeforeStart(fileName, lineNumber);

            inSubroutine = false;
            break;

        case LineParser::INSTRUCTION:
            if (!inSubroutine)
                throw Exception::InstructionLeak(fileName, lineNumber);

            if (!relocate) {
                subroutine->second.instructions.push_back(Instruction::createVerbatim(parser.instruction));
            } else {
                relocate = false;
            }
            break;

        case LineParser::RELOCATION:
            if (!inSubroutine)
                throw Exception::RelocationLeak(fileName, lineNumber);

            subroutine->second.dependencies.insert(parser.symbol);
            relocate = true;
            subroutine->second.instructions.push_back(Instruction::createRelocated(parser.symbol, parser.offset));
            break;

        case LineParser::BLANK:
            // do nothing
            break;
        }
    }

    if (inSubroutine)
        throw Exception::SubroutineNotEnded(fileName, lineNumber);
} catch (const Exception::ParserError& e) {
    std::stringstream ss;
    ss << "Parse error at " << e.getFileName() << ":" << e.getLineNumber();
    throw std::runtime_error(ss.str());
}

/**
 * Process defined subroutines and static variables and see which of them are
 * actually used.
 */
void resolveSymbols(const SubroutinesMap& definedSubroutines,
                    const StaticVariablesMap& definedStaticVariables,
                    std::vector<Subroutine>& usedSubroutines,
                    std::vector<StaticVariable>& usedStaticVariables)
{
    std::set<std::string> usedSymbols;
    std::stack<std::string> unresolvedSymbols;

    // Each program needs to define "entry symbol"
    unresolvedSymbols.push("__bootstrap");

    // While there are unresolved symbols
    while (!unresolvedSymbols.empty()) {
        const auto symbol = unresolvedSymbols.top();
        unresolvedSymbols.pop();

        assert(usedSymbols.count(symbol) == 0);
        // TODO: investigate if push_back'ed values can be std::move'd

        const auto subroutine = definedSubroutines.find(symbol);
        if (subroutine != definedSubroutines.end()) {
            // Symbol is a defined subroutine.
            usedSubroutines.push_back(subroutine->second);
            usedSymbols.insert(symbol);

            // For each of the subroutine dependencies...
            for (auto& dependency: subroutine->second.dependencies) {
                // ... append to unresolved symbols if not yet resolved
                if (usedSymbols.count(dependency) == 0) {
                    unresolvedSymbols.push(dependency);
                }
            }
        } else {
            // Symbol is not a defined subroutine
            const auto staticVariable = definedStaticVariables.find(symbol);
            if (staticVariable != definedStaticVariables.end()) {
                // Symbol is a defined static variable
                usedStaticVariables.push_back(staticVariable->second);
                usedSymbols.insert(symbol);
            } else {
                // Symbol is neither a defined static variable nor a defined
                // subroutine.
                throw std::runtime_error("unresolved symbol: " + symbol);
            }
        }
    }
}

/**
 * Write instructions for initializing static variable
 *
 * @param out is an output stream
 * @param address is an address allocated for static variable
 * @param data is data used for initialization
 *
 * @return generated code size in instructions
 */
unsigned writeStaticVariableInitializer(std::ostream& out,
                                        unsigned address,
                                        const std::vector<unsigned>& data)
{
    using namespace hcc::instruction;

    unsigned oldValue; // used for RLE compression
    unsigned size = 0;

    for (auto& value : data) {
        if (value == 0) {
            outputBinary(out, address++);
            outputBinary(out, COMPUTE | DEST_M | COMP_ZERO);
            size += 2;
        } else {
            // A-instruction is only applicable for values without MSB set.
            // Use double negation to bypass that limitation.
            bool negate = value & COMPUTE;

            // RLE compression
            if (value != oldValue) {
                outputBinary(out, negate ? ~value : value);
                outputBinary(out, COMPUTE | DEST_D | (negate ? COMP_NOT_A : COMP_A));
                size += 2;

                oldValue = value;
            }
            outputBinary(out, address++);
            outputBinary(out, COMPUTE | DEST_M | COMP_D);
            size += 2;
        }
    }
    return size;
}

int main(int argc, char *argv[])
try {
    bool showHelp = false;
    std::string outputFileName("output.hack"); // default output file name
    std::list<std::string> inputFileNames;

    // parse command line arguments
    for (int i = 1; i<argc; ++i) {
        if (std::strcmp(argv[i], "-o") == 0) {
            ++i;
            if (i<argc) {
                outputFileName = argv[i];
            } else {
                throw std::runtime_error("argument to '-o' is missing");
            }
        } else if (std::strcmp(argv[i], "-h") == 0) {
            showHelp = true;
        } else {
            inputFileNames.push_back(argv[i]);
        }
    }
    if (showHelp) {
        std::cout << "Usage: " << argv[0] << " [options] file...\n"
                  << "Options:\n"
                  << "  -h        show this information\n"
                  << "  -o <file> place the output into <file>\n";
        return 0;
    }
    if (inputFileNames.size() == 0) {
        throw std::runtime_error("no input files");
    }

    // scan pass
    SubroutinesMap definedSubroutines;
    StaticVariablesMap definedStaticVariables;
    std::set<std::string> definedSymbols;

    // scan each input file for subroutines and static variables
    for (const auto& inputFileName: inputFileNames) {
        parse(inputFileName, definedSymbols, definedSubroutines, definedStaticVariables);
    }

    // resolve the symbols
    std::vector<Subroutine> usedSubroutines;
    std::vector<StaticVariable> usedStaticVariables;
    resolveSymbols(definedSubroutines, definedStaticVariables,
                   usedSubroutines, usedStaticVariables);

    // production pass and allocation of addresses
    std::ofstream outputFile(outputFileName.c_str());
    AddressTable addressTable;
    unsigned romCounter = 0;
    unsigned ramCounter = 15;

    // first allocate (and initialize) the static variables
    for (const auto& staticVariable: usedStaticVariables) {
        const auto& data = staticVariable.data;

        addressTable[staticVariable.symbol] = ramCounter;
        if (data.size() == 0) {
            ++ramCounter;
        } else {
            romCounter += writeStaticVariableInitializer(outputFile, ramCounter, data);
            ramCounter += data.size();
        }
    }

    // next allocate the subroutines...
    for (const auto& subroutine: usedSubroutines) {
        addressTable[subroutine.symbol] = romCounter;
        romCounter += subroutine.instructions.size();
    }
    // ...and write them
    for (const auto& subroutine: usedSubroutines) {
        subroutine.write(outputFile, addressTable);
    }

    return 0;
} catch (const std::runtime_error& e) {
    std::cerr << "ld: " << e.what() << '\n';
    return 1;
}

