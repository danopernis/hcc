// Copyright (c) 2013 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include "control_flow_graph.h"
#include <boost/algorithm/string/join.hpp>
#include <sstream>
#include <stdexcept>

namespace hcc { namespace ssa {

std::ostream& operator<<(std::ostream& os, const unit& u)
{
    for (const auto& global : u.globals) {
        os << "global " << global << "\n";
    }
    for (const auto& subroutine : u.subroutines) {
        os << "define " << subroutine.first << "\n";
        for (const auto& instr : subroutine.second.instructions) {
            if (instr.type != instruction_type::LABEL)
                os << "\t";

            os << instr << "\n";
        }
    }
    return os;
}

void unit::load(std::istream& input)
{
    unsigned line_number = 0;
    std::string line;
    subroutine_map::iterator current_subroutine;

    while (std::getline(input, line)) {
        ++line_number;
        if (line.empty())
            continue;

        std::string token;
        std::stringstream line_stream(line);
        line_stream >> token;

        // structure
        if (token == "global") {
            line_stream >> token;
            globals.insert(token);
            continue;
        } else if (token == "define") {
            line_stream >> token;
            current_subroutine = insert_subroutine(token);
            continue;
        }

        // instruction type
        instruction_type type;
        if (token == "block") {
            type = instruction_type::LABEL;
        } else if (token == "argument") {
            type = instruction_type::ARGUMENT;
        } else if (token == "branch") {
            type = instruction_type::BRANCH;
        } else if (token == "jump") {
            type = instruction_type::JUMP;
        } else if (token == "call") {
            type = instruction_type::CALL;
        } else if (token == "return") {
            type = instruction_type::RETURN;
        } else if (token == "load") {
            type = instruction_type::LOAD;
        } else if (token == "store") {
            type = instruction_type::STORE;
        } else if (token == "mov") {
            type = instruction_type::MOV;
        } else if (token == "add") {
            type = instruction_type::ADD;
        } else if (token == "sub") {
            type = instruction_type::SUB;
        } else if (token == "and") {
            type = instruction_type::AND;
        } else if (token == "or") {
            type = instruction_type::OR;
        } else if (token == "lt") {
            type = instruction_type::LT;
        } else if (token == "gt") {
            type = instruction_type::GT;
        } else if (token == "eq") {
            type = instruction_type::EQ;
        } else if (token == "neg") {
            type = instruction_type::NEG;
        } else if (token == "not") {
            type = instruction_type::NOT;
        } else if (token == "phi") {
            type = instruction_type::PHI;
        } else {
            std::stringstream ss;
            ss << "Unexpected '" << token << "' at line " << line_number;
            throw std::runtime_error(ss.str());
        }

        // instruction arguments
        std::vector<std::string> args;
        while (true) {
            line_stream >> token;
            if (token.empty())
                break;

            args.push_back(std::move(token));
        }
        current_subroutine->second.instructions.emplace_back(type, args);
    }
}


const std::map<instruction_type, std::string> type_to_string = {
    { instruction_type::ARGUMENT,   "argument " },
    { instruction_type::BRANCH,     "branch " },
    { instruction_type::JUMP,       "jump " },
    { instruction_type::CALL,       "call " },
    { instruction_type::RETURN,     "return " },
    { instruction_type::LOAD,       "load " },
    { instruction_type::STORE,      "store " },
    { instruction_type::MOV,        "mov " },
    { instruction_type::ADD,        "add " },
    { instruction_type::SUB,        "sub " },
    { instruction_type::AND,        "and " },
    { instruction_type::OR,         "or " },
    { instruction_type::LT,         "lt " },
    { instruction_type::GT,         "gt " },
    { instruction_type::EQ,         "eq " },
    { instruction_type::NEG,        "neg " },
    { instruction_type::NOT,        "not " },
    { instruction_type::PHI,        "phi " },
    { instruction_type::LABEL,      "block " }
};

std::ostream& operator<<(std::ostream& os, const instruction& instr)
{
    os << type_to_string.at(instr.type) << boost::algorithm::join(instr.arguments, " ");
    return os;
}

void instruction::use_apply(std::function<void(std::string&)> g)
{
    auto f = [&] (std::string& s) {
        if (!s.empty() && s[0] == '%')
            g(s);
    };

    switch (type) {
    case instruction_type::CALL: {
        int counter = 0;
        for (auto& argument : arguments) {
            if (counter >= 2) {
                f(argument);
            }
            ++counter;
        }
        } break;
    case instruction_type::PHI: {
        int counter = 0;
        for (auto& argument : arguments) {
            if (counter != 0 && counter % 2 == 0) {
                f(argument);
            }
            ++counter;
        }
        } break;
    case instruction_type::RETURN:
    case instruction_type::BRANCH:
        f(arguments[0]);
        break;
    case instruction_type::LABEL:
    case instruction_type::JUMP:
    case instruction_type::ARGUMENT:
        break;
    case instruction_type::STORE:
        f(arguments[0]);
        f(arguments[1]);
        break;
    case instruction_type::ADD:
    case instruction_type::SUB:
    case instruction_type::AND:
    case instruction_type::OR:
    case instruction_type::LT:
    case instruction_type::GT:
    case instruction_type::EQ:
        f(arguments[1]);
        f(arguments[2]);
        break;
    case instruction_type::LOAD:
    case instruction_type::MOV:
    case instruction_type::NEG:
    case instruction_type::NOT:
        f(arguments[1]);
        break;
    default:
        assert(false);
    }
}

void instruction::def_apply(std::function<void(std::string&)> f)
{
    switch (type) {
    case instruction_type::BRANCH:
    case instruction_type::JUMP:
    case instruction_type::RETURN:
    case instruction_type::LABEL:
    case instruction_type::STORE:
        break;
    case instruction_type::CALL:
    case instruction_type::LOAD:
    case instruction_type::MOV:
    case instruction_type::NEG:
    case instruction_type::NOT:
    case instruction_type::ADD:
    case instruction_type::SUB:
    case instruction_type::AND:
    case instruction_type::OR:
    case instruction_type::LT:
    case instruction_type::GT:
    case instruction_type::EQ:
    case instruction_type::PHI:
    case instruction_type::ARGUMENT:
        f(arguments[0]);
        break;
    default:
        assert(false);
    }
}

void instruction::label_apply(std::function<void(std::string&)> f)
{
    switch (type) {
    case instruction_type::LABEL:
    case instruction_type::JUMP:
        f(arguments[0]);
        break;
    case instruction_type::BRANCH:
        f(arguments[1]);
        f(arguments[2]);
        break;
    case instruction_type::PHI: {
        int counter = 0;
        for (auto& argument : arguments) {
            if (counter % 2 == 1) {
                f(argument);
            }
            ++counter;
        }
        } break;
    case instruction_type::RETURN:
    case instruction_type::STORE:
    case instruction_type::CALL:
    case instruction_type::LOAD:
    case instruction_type::MOV:
    case instruction_type::NEG:
    case instruction_type::NOT:
    case instruction_type::ADD:
    case instruction_type::SUB:
    case instruction_type::AND:
    case instruction_type::OR:
    case instruction_type::LT:
    case instruction_type::GT:
    case instruction_type::EQ:
    case instruction_type::ARGUMENT:
        break;
    default:
        assert(false);
    }
}

}} // end namespace hcc::ssa
