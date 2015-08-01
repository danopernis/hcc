// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "asm.h"
#include "jack_parser.h"
#include "jack_tokenizer.h"
#include "ssa.h"
#include <boost/algorithm/string/join.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace hcc::jack;
using namespace hcc::ssa;

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
                throw std::runtime_error(
                    std::string("Unknown command line option: ") +
                    static_cast<char>(optopt));
            case ':':
                throw std::runtime_error(
                    std::string("Missing argument for command line option: ") +
                    static_cast<char>(optopt));
            }
        }
        while (optind < argc) {
            input_files.emplace_back(argv[optind++]);
        }
        if (input_files.empty() && !help) {
            throw std::runtime_error("Missing input file(s)");
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
        std::cout <<
            "Usage: hcc [options] file...\n"
            "Options:\n"
            "  -h                   Display this information\n"
            "  -o <file>            Place the output into <file>\n"
            "  -S                   Compile only; do not assemble\n"
            ;
    }

    bool help {false};
    bool assemble {true};
    std::string output;
    std::vector<std::string> input_files;
};

int main(int argc, char* argv[])
try {
    const command_line_options options {argc, argv};
    if (options.help) {
        options.print_help();
        return 0;
    }

    // parse input
    std::vector<ast::Class> classes;
    try {
        for (const auto& input_file : options.input_files) {
            std::ifstream input {input_file};
            tokenizer t {
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>()};
            classes.push_back(parse(t));
        }
    } catch (const parse_error& e) {
        std::stringstream ss;
        ss  << "Parse error: " << e.what()
            << " at " << e.line << ":" << e.column;
        throw std::runtime_error(ss.str());
    }

    // produce intermediate code
    unit u;
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

    hcc::asm_program out;
    u.translate_to_asm(out);
    out.local_optimization();

    // output
    if (options.assemble) {
        hcc::saveHACK(options.output, out.assemble());
    } else {
        out.save(options.output);
    }

    return 0;
} catch (const std::runtime_error& e) {
    std::cerr
        << "When executing "
        << boost::algorithm::join(std::vector<std::string>(argv, argv + argc), " ")
        << " ...\n" << e.what() << "\n";
    return 1;
}
