// Copyright (c) 2014 Dano Pernis
// See LICENSE.txt

#ifndef CONTROL_FLOW_GRAPH_H
#define CONTROL_FLOW_GRAPH_H

#include "ssa.h"
#include "graph.h"
#include "graph_dominance.h"
#include <cassert>
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace hcc { namespace ssa {

// Represents a range of instructions
// Iterators are inclusive on construction; however end() iterator points just
// behind the range.
class instruction_list_range {
public:
    instruction_list_range() {}

    instruction_list_range(
            instruction_list::iterator first,
            instruction_list::iterator last)
        : first{ std::move(first) }
        , last{ std::move(last) }
    { }

    instruction_list::iterator begin() const
    { return first; }

    instruction_list::iterator end() const
    { return ++instruction_list::iterator(last); }

private:
    instruction_list::iterator first;
    instruction_list::iterator last;
};

struct control_flow_graph {
    control_flow_graph(instruction_list& instructions)
    {
        // entry node is the first node there is
        assert (!instructions.empty());
        assert (instructions.front().type == instruction_type::LABEL);
        assert (instructions.front().arguments.size()  == 1);
        entry_node_ = add_node(instructions.front().arguments[0]);

        // exit node is invented
        exit_node_ = add_node("EXIT");
        exit_node_instructions_.emplace_back(instruction_type::LABEL, std::vector<std::string>({"EXIT"}));
        nodes[exit_node_] = instruction_list_range(exit_node_instructions_.begin(), exit_node_instructions_.begin());

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
                nodes[current_node] = instruction_list_range(current_node_start, i);
                break;
            case instruction_type::BRANCH:
                assert (i->arguments.size() == 3);

                g.add_edge(current_node, add_node(i->arguments[1]));
                g.add_edge(current_node, add_node(i->arguments[2]));

                assert (0 <= current_node && current_node < static_cast<int>(nodes.size()));
                nodes[current_node] = instruction_list_range(current_node_start, i);
                break;
            case instruction_type::RETURN:
                assert (0 <= current_node && current_node < static_cast<int>(nodes.size()));
                nodes[current_node] = instruction_list_range(current_node_start, i);
                g.add_edge(current_node, exit_node_);
                break;
            default:
                // nothing to do here
                break;
            }
        }

        dominance_.reset(new graph_dominance(g, entry_node_));
        reverse_dominance_.reset(new graph_dominance(g.reverse(), exit_node_));
    }

    /** Accessors */
    const graph_dominance& dominance() const { return *dominance_; }
    const graph_dominance& reverse_dominance() const { return *reverse_dominance_; }
    int entry_node() const { return entry_node_; }
    int exit_node() const { return exit_node_; }

    std::vector<instruction_list_range> nodes;

    const std::vector<std::set<int>>& successors() const { return g.successors(); }
    std::map<int, std::string> index_to_name;
    std::map<std::string, int> name_to_index;
    graph g;

private:
    std::unique_ptr<graph_dominance> dominance_;
    std::unique_ptr<graph_dominance> reverse_dominance_;
    int entry_node_;
    int exit_node_;
    instruction_list exit_node_instructions_;

    int add_node(std::string name)
    {
        auto it = name_to_index.find(name);
        if (it == name_to_index.end()) {
            int index = g.add_node();
            name_to_index.emplace(name, index);
            index_to_name.emplace(index, name);
            nodes.emplace_back();
            return index;
        } else {
            return it->second;
        }
    }
};

}} // namespace hcc::ssa

#endif // CONTROL_FLOW_GRAPH_H
