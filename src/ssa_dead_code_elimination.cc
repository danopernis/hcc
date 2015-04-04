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
        i->basic_block = bb.name;
        switch (i->type) {
        case instruction_type::JUMP:
        case instruction_type::JLT:
        case instruction_type::JEQ:
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
        w->use_apply([&] (argument& a_use) {
            if (!a_use.is_reg()) {
                return;
            }
            const auto& use = a_use.get_reg();
            // ...find the definer...
            for_each_bb([&] (basic_block& bb) {
            for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e; ++i) {
                i->def_apply([&] (argument& a_def) {
                    if (!a_def.is_reg()) {
                        return;
                    }
                    const auto& def = a_def.get_reg();

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
        if (!i->mark) {
            i = bb.instructions.erase(i);
        } else {
            ++i;
        }
    }
    });
}

}} // namespace hcc::ssa
