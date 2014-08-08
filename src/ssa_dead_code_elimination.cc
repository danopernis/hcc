// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include <algorithm>
#include "ssa.h"

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

void subroutine::dead_code_elimination()
{
    recompute_control_flow_graph();

    std::stack<std::pair<instruction_list::iterator, int>> worklist; // instruction and it's block
    for (auto node = 0; node < static_cast<int>(nodes.size()); ++node) {
        for (auto instruction = nodes[node].begin(), e = nodes[node].end(); instruction != e; ++instruction) {
            instruction->mark = false;
            if (prelive(*instruction)) {
                instruction->mark = true;
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
            for (auto node = 0; node < static_cast<int>(nodes.size()); ++node) {
                for (auto instruction = nodes[node].begin(), e = nodes[node].end(); instruction != e; ++instruction) {
                    bool assigned = false;
                    instruction->def_apply([&] (std::string& s) { assigned = assigned || (s == v); });
                    if (assigned) {
                        //... maybe set live and append to worklist
                        if (!instruction->mark) {
                            instruction->mark = true;;
                            worklist.push(std::make_pair(instruction, node));
                        }
                    }
                }
            }
        }


        // for each y in r_dfs(x)
        for (int b : reverse_dominance->dfs[p.second]) {
            auto last = --nodes[b].end();
            if (!last->mark) {
                last->mark = true;
                worklist.push(std::make_pair(last, b));
            }
        }
    }

    // sweep
    for (auto i = instructions.begin(), e = instructions.end(); i != e;) {
        if (i->type != instruction_type::LABEL && !i->mark) {
            i = instructions.erase(i);
        } else {
            ++i;
        }
    }
}

}} // namespace hcc::ssa
