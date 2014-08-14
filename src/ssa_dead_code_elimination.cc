// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include <algorithm>
#include "ssa.h"

namespace hcc { namespace ssa {

void subroutine::dead_code_elimination()
{
    recompute_control_flow_graph();

    std::stack<std::pair<instruction_list::iterator, int>> worklist; // instruction and its block
    for (auto& block : nodes) {
        for (auto i = block.begin(), e = block.end(); i != e; ++i) {
            switch (i->type) {
            case instruction_type::JUMP:
            case instruction_type::BRANCH:
            case instruction_type::CALL:
            case instruction_type::RETURN:
            case instruction_type::LOAD:
            case instruction_type::STORE:
                i->mark = true;
                worklist.emplace(i, block.index);
                break;
            default:
                i->mark = false;
            }
        }
    }

    while (!worklist.empty()) {
        auto w = worklist.top();
        worklist.pop();

        // for each use...
        w.first->use_apply([&] (std::string& use) {
            // ...find the definer...
            for (auto& block : nodes) {
                for (auto i = block.begin(), e = block.end(); i != e; ++i) {
                    i->def_apply([&] (std::string& def) {
                        if (def == use && !i->mark) {
                            // ... mark and append to worklist
                            i->mark = true;;
                            worklist.emplace(i, block.index);
                        }
                    });
                }
            }
        });

        // for each y in r_dfs(x)
        for_each_dfs(w.second, [&] (basic_block& block) {
            auto i = block.end();
            if (!i->mark) {
                i->mark = true;
                worklist.emplace(i, block.index);
            }
        });
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
