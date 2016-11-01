// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <set>
#include <string>

namespace hcc {
namespace ssa {
namespace {

/** Use dominance frontier set to find where phi-functions are needed */
void insert_temp_phi(subroutine& s)
{
    std::set<reg> globals;
    s.for_each_bb([&](basic_block& bb) {
        std::set<reg> varkill;
        for (auto& i : bb.instructions) {
            i.use_apply([&](argument& a) {
                if (a.is_reg()) {
                    const auto r = a.get_reg();
                    if (!varkill.count(r)) {
                        globals.insert(r);
                    }
                }
            });
            i.def_apply([&](argument& a) {
                const auto r = a.get_reg();
                varkill.insert(r);
            });
        }
    });

    // init
    int iteration = 0;
    s.for_each_bb([&](basic_block& block) {
        block.has_already = iteration;
        block.work = iteration;
    });

    std::set<label> w; // worklist of CFG nodes being processed
    for (const auto& variable : globals) {
        ++iteration;

        s.for_each_bb([&](basic_block& block) {
            for (auto& instruction : block.instructions) {
                instruction.def_apply([&](argument& a) {
                    if (a.get_reg() == variable) {
                        // add to worklist
                        block.work = iteration;
                        w.emplace(block.name);
                    }
                });
            }
        });

        while (!w.empty()) {
            auto x = *w.begin();
            w.erase(w.begin());

            s.for_each_bb_in_dfs(x, [&](basic_block& block) {
                if (block.has_already < iteration) {
                    // place PHI
                    block.instructions.emplace_front(
                        instruction(instruction_type::PHI, {variable}));

                    block.has_already = iteration;
                    if (block.work < iteration) {
                        block.work = iteration;
                        w.emplace(block.name);
                    }
                }
            });
        }
    }
}

struct name_manager {
    name_manager(subroutine_ir& subroutine_, const std::set<reg>& variables)
        : subroutine(subroutine_)
    {
        for (const auto& v : variables) {
            entries[v].emplace(v);
            backref.emplace(v, v);
        }
    }

    reg current_name(const reg& name) { return entries.at(backref.at(name)).top(); }

    reg new_name(const reg& name)
    {
        const auto a = backref.at(name);
        const auto r = subroutine.create_reg();
        subroutine.add_debug(r, "ssa_construction", a);
        entries.at(a).emplace(r);
        backref.emplace(r, a);
        return r;
    }

    void pop(const reg& name) { entries.at(backref.at(name)).pop(); }

private:
    std::map<reg, std::stack<reg>> entries;
    std::map<reg, reg> backref;
    subroutine_ir& subroutine;
};

} // namespace {

// algorithm is due to Cytron et al.
void subroutine::construct_minimal_ssa()
{
    // Initialization
    const auto variables = collect_variable_names();

    // Step 1: insert (incomplete) phi-functions
    insert_temp_phi(*this);

    // Step 2: append indices to variable names
    name_manager names(*this, variables);
    std::function<void(basic_block&)> rename = [&](basic_block& x) {
        for (auto& instr : x.instructions) {
            if (instr.type != instruction_type::PHI) {
                // Uses get current name
                instr.use_apply([&](argument& a) {
                    if (a.is_reg()) {
                        a = names.current_name(a.get_reg());
                    }
                });
            }
            // Definitions get new name
            instr.def_apply([&](argument& a) { a = names.new_name(a.get_reg()); });
        }

        // Complete uses in temporary phi-function using current names
        for_each_cfg_successor(x.name, [&](basic_block& y) {
            for (auto& instr : y.instructions) {
                if (instr.type == instruction_type::PHI) {
                    auto dest = instr.arguments[0].get_reg();
                    instr.arguments.push_back(x.name);
                    instr.arguments.push_back(names.current_name(dest));
                }
            }
        });

        // Recursive call
        for_each_domtree_successor(x.name, rename);

        // Cleanup
        for (auto& instr : x.instructions) {
            instr.def_apply([&](argument& a) { names.pop(a.get_reg()); });
        }
    };
    rename(entry_node());
}

} // namespace ssa {
} // namespace hcc {
