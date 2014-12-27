// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include <algorithm>
#include "ssa.h"

namespace hcc { namespace ssa {

void subroutine::dead_code_elimination()
{
    recompute_dominance();

    std::stack<instruction_list::iterator> worklist;
    for_each_bb([&] (basic_block& bb) {
    for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e; ++i) {
        i->basic_block = bb.index;
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
    });

    while (!worklist.empty()) {
        auto w = worklist.top();
        worklist.pop();

        // for each use...
        w->use_apply([&] (std::string& use) {
            // ...find the definer...
            for_each_bb([&] (basic_block& bb) {
            for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e; ++i) {
                i->def_apply([&] (std::string& def) {
                    if (def == use && !i->mark) {
                        // ... mark and append to worklist
                        i->mark = true;
                        worklist.emplace(i);
                    }
                });
            }
            });
        });

        for_each_reverse_dfs(w->basic_block, [&] (basic_block& block) {
            if (block.instructions.empty()) {
                return;
            }
            auto i = --block.instructions.end();
            if (!i->mark) {
                i->mark = true;
                worklist.emplace(i);
            }
        });
    }

    // sweep
    for_each_bb([&] (basic_block& bb) {
    for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e;) {
        if (i->type != instruction_type::LABEL && !i->mark) {
            i = bb.instructions.erase(i);
        } else {
            ++i;
        }
    }
    });
}

}} // namespace hcc::ssa
