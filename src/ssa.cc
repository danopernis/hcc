// Copyright (c) 2013-2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <cassert>

namespace hcc { namespace ssa {

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

void subroutine::recompute_control_flow_graph()
{
    nodes.clear();
    std::map<std::string, int> name_to_index;
    g = graph();

    auto add_node = [&] (const std::string& name)
    {
        auto it = name_to_index.find(name);
        if (it == name_to_index.end()) {
            int index = g.add_node();
            name_to_index.emplace(name, index);
            nodes.emplace_back();
            nodes.back().name = name;
            return index;
        } else {
            return it->second;
        }
    };

    // entry node is the first node there is
    assert (!instructions.empty());
    assert (instructions.front().type == instruction_type::LABEL);
    assert (instructions.front().arguments.size()  == 1);
    entry_node = add_node(instructions.front().arguments[0]);

    // exit node is invented
    exit_node = add_node("EXIT");
    exit_node_instructions.emplace_back(instruction_type::LABEL, std::vector<std::string>({"EXIT"}));
    nodes[exit_node].first = exit_node_instructions.begin();
    nodes[exit_node].last = exit_node_instructions.begin();

    int current_node = -1;
    instruction_list::iterator current_node_start;

    for (auto i = instructions.begin(), e = instructions.end(); i != e; ++i) {
        switch (i->type) {
        case instruction_type::LABEL:
            assert (i->arguments.size() == 1);

            current_node = add_node(i->arguments[0]);
            current_node_start = i;
            break;
        case instruction_type::JUMP:
            assert (i->arguments.size() == 1);

            g.add_edge(current_node, add_node(i->arguments[0]));

            assert (0 <= current_node && current_node < static_cast<int>(nodes.size()));
            nodes[current_node].first = current_node_start;
            nodes[current_node].last = i;
            break;
        case instruction_type::BRANCH:
            assert (i->arguments.size() == 3);

            g.add_edge(current_node, add_node(i->arguments[1]));
            g.add_edge(current_node, add_node(i->arguments[2]));

            assert (0 <= current_node && current_node < static_cast<int>(nodes.size()));
            nodes[current_node].first = current_node_start;
            nodes[current_node].last = i;
            break;
        case instruction_type::RETURN:
            assert (0 <= current_node && current_node < static_cast<int>(nodes.size()));
            nodes[current_node].first = current_node_start;
            nodes[current_node].last = i;
            g.add_edge(current_node, exit_node);
            break;
        default:
            // nothing to do here
            break;
        }
    }

    dominance.reset(new graph_dominance(g, entry_node));
    reverse_dominance.reset(new graph_dominance(g.reverse(), exit_node));
}

}} // end namespace hcc::ssa
