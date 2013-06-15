// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include <algorithm>
#include "ssa_dead_code_elimination.h"
#include "control_flow_graph.h"

namespace hcc { namespace ssa {

bool prelive(const instruction& instr)
{
    switch (instr.type) {
    case instruction_type::JUMP:
    case instruction_type::BRANCH:
    case instruction_type::CALL:
    case instruction_type::RETURN:
    case instruction_type::LOAD:
    case instruction_type::STORE:
        return true;
    default:
        return false;
    }
}

void dead_code_elimination(instruction_list& instructions)
{
    control_flow_graph cfg(instructions);

    std::vector<instruction_list::iterator> live;
    std::stack<std::pair<instruction_list::iterator, int>> worklist; // instruction and it's block
    for (auto node = 0; node < static_cast<int>(cfg.nodes.size()); ++node) {
        for (auto instruction = cfg.nodes[node].begin(), e = cfg.nodes[node].end(); instruction != e; ++instruction) {
            if (prelive(*instruction)) {
                live.push_back(instruction);
                worklist.push(std::make_pair(instruction, node));
            }
        }
    }

    while (!worklist.empty()) {
        auto p = worklist.top();
        auto o = p.first;
        worklist.pop();

        // for each definer...
        std::set<std::string> used_variables;
        o->use_apply([&](std::string& s) { used_variables.insert(s); });
        for (const auto& v : used_variables) {
            for (auto node = 0; node < static_cast<int>(cfg.nodes.size()); ++node) {
                for (auto instruction = cfg.nodes[node].begin(), e = cfg.nodes[node].end(); instruction != e; ++instruction) {
                    bool assigned = false;
                    instruction->def_apply([&] (std::string& s) { assigned = assigned || (s == v); });
                    if (assigned) {
                        //... maybe set live and append to worklist
                        if (std::find(live.begin(), live.end(), instruction) == live.end()) { // TODO faster!
                            live.push_back(instruction);
                            worklist.push(std::make_pair(instruction, node));
                        }
                    }
                }
            }
        }


        // for each y in r_dfs(x)
        for (int b : cfg.reverse_dominance().dfs[p.second]) {
            auto last = --cfg.nodes[b].end();
            if (std::find(live.begin(), live.end(), last) == live.end()) { // TODO faster!
                live.push_back(last);
                worklist.push(std::make_pair(last, b));
            }
        }
    }

    // sweep
    for (auto i = instructions.begin(), e = instructions.end(); i != e;) {
        if (i->type != instruction_type::LABEL && std::find(live.begin(), live.end(), i) == live.end()) { // TODO faster!
            i = instructions.erase(i);
        } else {
            ++i;
        }
    }
}

}} // namespace hcc::ssa
