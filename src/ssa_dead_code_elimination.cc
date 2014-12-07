// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include <algorithm>
#include "ssa.h"

namespace hcc { namespace ssa {

void subroutine::dead_code_elimination()
{
    recompute_control_flow_graph();

    std::stack<instruction_list::iterator> worklist;
    for (auto i = instructions.begin(), e = instructions.end(); i != e; ++i) {
        switch (i->type) {
        case instruction_type::JUMP:
        case instruction_type::BRANCH:
        case instruction_type::CALL:
        case instruction_type::RETURN:
        case instruction_type::LOAD:
        case instruction_type::STORE:
            i->mark = true;
            worklist.emplace(i);
            break;
        default:
            i->mark = false;
        }
    }

    while (!worklist.empty()) {
        auto w = worklist.top();
        worklist.pop();

        // for each use...
        w->use_apply([&] (std::string& use) {
            // ...find the definer...
            for (auto i = instructions.begin(), e = instructions.end(); i != e; ++i) {
                i->def_apply([&] (std::string& def) {
                    if (def == use && !i->mark) {
                        // ... mark and append to worklist
                        i->mark = true;
                        worklist.emplace(i);
                    }
                });
            }
        });

        for_each_reverse_dfs(w->basic_block, [&] (basic_block& block) {
            auto i = block.end();
            if (!i->mark) {
                i->mark = true;
                worklist.emplace(i);
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
