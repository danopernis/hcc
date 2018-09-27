// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#include <algorithm>
#include "ssa.h"

namespace hcc {
namespace ssa {

void subroutine::dead_code_elimination()
{
    // Dead code elimination (DCE) eliminates all non-critical instructions.
    // Critical instruction is any instruction capable of changing observable
    // behavior of given subroutine. DCE works by first marking the implicitly
    // critical instructions and then iteratively broadening a set of critical
    // instructions until no more instructions can be added. To this end,
    // instrutions are pushed into and pulled from a worklist. As soon as the
    // marking algorithm converges, subsequest pass sweeps all non-critical
    // instructions and DCE ends.

    // current iteration of the algorithm
    auto work = 1;

    // first iteration
    for_each_bb([&](basic_block& bb) {
        for (auto& i : bb.instructions) {
            switch (i.type) {
            case instruction_type::JUMP:
            case instruction_type::CALL:
            case instruction_type::RETURN:
            case instruction_type::STORE:
                i.work = work;
                break;
            default:
                i.work = 0;
            }
        }
    });

    // iterate until marking converges
    bool changed = true;
    while (changed) {
        changed = false;

        std::set<reg> uses;
        std::set<label> bbs;

        for_each_bb([&](basic_block& bb) {
            for (auto& i : bb.instructions) {
                // if the instruction is on the worklist, collect dependencies
                if (i.work == work) {
                    // collect registers
                    i.use_apply([&](argument& a) {
                        if (a.is_reg()) {
                            uses.emplace(a.get_reg());
                        }
                    });
                    // collect basic blocks
                    for_each_reverse_dfs(bb.name,
                                         [&](basic_block& block) { bbs.emplace(block.name); });
                }
            }
        });

        // next iteration
        ++work;

        for_each_bb([&](basic_block& block) {
            if (block.instructions.empty()) {
                return;
            }

            // put terminator of collected basic block on the worklist
            auto i = --block.instructions.end();
            if (bbs.count(block.name) && i->work == 0) {
                i->work = work;
                changed = true;
            }

            // put definition of collected register on the worklist
            for (auto& i : block.instructions) {
                i.def_apply([&](argument& def) {
                    if (uses.count(def.get_reg()) && i.work == 0) {
                        i.work = work;
                        changed = true;
                    }
                });
            }
        });
    }

    // sweep
    for_each_bb([&](basic_block& bb) {
        for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e;) {
            if (i->work == 0) {
                i = bb.instructions.erase(i);
            } else {
                ++i;
            }
        }
    });
}

} // namespace ssa {
} // namespace hcc {
