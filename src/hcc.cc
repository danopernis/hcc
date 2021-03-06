// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#include "hcc/assembler/asm.h"
#include "hcc/jack/ast.h"
#include "hcc/jack/parser.h"
#include "hcc/jack/tokenizer.h"
#include "hcc/ssa/ssa.h"
#include "hcc/vm/parser.h"
#include "hcc/vm/optimize.h"
#include "hcc/vm/writer.h"
#include <boost/algorithm/string/join.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

bool ends_with(const std::string& value, const std::string& suffix)
{
    if (suffix.size() > value.size()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

struct command_line_options {
    command_line_options(int argc, char* argv[])
    {
        opterr = 0;
        int opt = -1;
        while ((opt = getopt(argc, argv, ":ho:S")) != -1) {
            switch (opt) {
            case 'h':
                help = true;
                break;
            case 'o':
                output = optarg;
                break;
            case 'S':
                assemble = false;
                break;
            case '?':
                throw std::runtime_error(std::string("Unknown command line option: ")
                                         + static_cast<char>(optopt));
            case ':':
                throw std::runtime_error(std::string("Missing argument for command line option: ")
                                         + static_cast<char>(optopt));
            }
        }
        while (optind < argc) {
            std::string input_file = argv[optind++];
            if (ends_with(input_file, ".jack")) {
                jack_input_files.emplace_back(input_file);
            } else if (ends_with(input_file, ".asm")) {
                asm_input_files.emplace_back(input_file);
            } else if (ends_with(input_file, ".vm")) {
                vm_input_files.emplace_back(input_file);
            } else {
                throw std::runtime_error("Input file has unknown suffix: " + input_file);
            }
        }

        const auto all_empty = jack_input_files.empty() && asm_input_files.empty()
                               && vm_input_files.empty();
        if (all_empty && !help) {
            throw std::runtime_error("Missing input file(s)");
        }

        // Until we get linker working, we have to impose some constraints
        if (asm_input_files.size() > 1) {
            throw std::runtime_error("More than one asm input file");
        }
        if (!asm_input_files.empty() && !jack_input_files.empty()) {
            throw std::runtime_error("Mixing jack and asm input files");
        }
        if (!jack_input_files.empty() && !vm_input_files.empty()) {
            throw std::runtime_error("Mixing jack and vm input files");
        }
        if (!vm_input_files.empty() && !asm_input_files.empty()) {
            throw std::runtime_error("Mixing vm and asm input files");
        }

        if (output.empty()) {
            if (assemble) {
                output = "output.hack";
            } else {
                output = "output.asm";
            }
        }
    }

    void print_help() const
    {
        std::cout << "Usage: hcc [options] file...\n"
                     "Options:\n"
                     "  -h                   Display this information\n"
                     "  -o <file>            Place the output into <file>\n"
                     "  -S                   Compile only; do not assemble\n";
    }

    bool help{false};
    bool assemble{true};
    std::string output;
    std::vector<std::string> jack_input_files;
    std::vector<std::string> asm_input_files;
    std::vector<std::string> vm_input_files;
};

void jack_to_asm(const std::vector<std::string>& jack_input_files, hcc::assembler::program& out)
{
    if (jack_input_files.empty()) {
        return;
    }

    // parse input
    std::vector<hcc::jack::Class> classes;
    try {
        for (const auto& input_file : jack_input_files) {
            std::ifstream input{input_file};
            hcc::jack::tokenizer t{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
            classes.push_back(parse(t));
        }
    }
    catch (const hcc::jack::parse_error& e) {
        std::stringstream ss;
        ss << "Parse error: " << e.what() << " at " << e.line << ":" << e.column;
        throw std::runtime_error(ss.str());
    }

    // produce intermediate code
    hcc::ssa::unit u;
    for (const auto& class_ : classes) {
        u.translate_from_jack(class_);
    }

    // optimize
    for (auto& subroutine_entry : u.subroutines) {
        auto& subroutine = subroutine_entry.second;
        subroutine.dead_code_elimination();
        subroutine.copy_propagation();
        subroutine.dead_code_elimination();
        subroutine.ssa_deconstruct();
        subroutine.allocate_registers();
    }

    u.translate_to_asm(out);
}

void vm_to_asm(const std::vector<std::string>& vm_input_files, hcc::assembler::program& out)
{
    if (vm_input_files.empty()) {
        return;
    }

    hcc::vm::writer writer(out);
    writer.writeBootstrap();
    for (const auto& input_file : vm_input_files) {
        std::ifstream input{input_file.c_str()};
        hcc::vm::parser parser{input};
        auto cmds = parser.parse();
        hcc::vm::optimize(cmds);
        writer.writeFile(input_file, cmds);
    }
}

void asm_to_asm(const std::vector<std::string>& asm_input_files, hcc::assembler::program& out)
{
    if (asm_input_files.empty()) {
        return;
    }

    for (const auto& input_file : asm_input_files) {
        std::ifstream input_stream{input_file};
        hcc::assembler::program prog{input_stream};
        out = prog;
    }
}

int main(int argc, char* argv[]) try {
    const command_line_options options{argc, argv};
    if (options.help) {
        options.print_help();
        return 0;
    }

    hcc::assembler::program out;
    jack_to_asm(options.jack_input_files, out);
    asm_to_asm(options.asm_input_files, out);
    vm_to_asm(options.vm_input_files, out);
    out.local_optimization();

    // output
    if (options.assemble) {
        hcc::assembler::saveHACK(options.output, out.assemble());
    } else {
        out.save(options.output);
    }

    return 0;
}
catch (const std::runtime_error& e) {
    std::cerr << "When executing "
              << boost::algorithm::join(std::vector<std::string>(argv, argv + argc), " ")
              << " ...\n" << e.what() << "\n";
    return 1;
}
