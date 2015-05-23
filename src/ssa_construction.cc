// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <set>
#include <string>

namespace hcc { namespace ssa {

namespace {

/** Use dominance frontier set to find where phi-functions are needed */
void insert_temp_phi(
    const std::set<reg>& variables,
    subroutine& s)
{
    // init
    int iteration = 0;
    s.for_each_bb([&] (basic_block& block) {
        block.has_already = iteration;
        block.work = iteration;
    });

    std::set<label> w; // worklist of CFG nodes being processed
    for (const auto& variable : variables) {
        ++iteration;

        s.for_each_bb([&] (basic_block& block) {
            for (auto& instruction : block.instructions) {
                instruction.def_apply([&] (argument& a) {
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

            s.for_each_bb_in_dfs(x, [&] (basic_block& block) {
                if (block.has_already < iteration) {
                    // place PHI
                    block.instructions.emplace_front(instruction(
                        instruction_type::PHI, {variable}));

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
    name_manager(regs_table& regs, const std::set<reg>& variables)
        : regs(regs)
    {
        for (const auto& v : variables) {
            entries.emplace(v, map_entry(v));
            backref.emplace(v, v);
        }
    }

    reg current_name(const reg& name)
    {
        return entries.at(backref.at(name)).current_name();
    }

    reg new_name(const reg& name)
    {
        const auto a = backref.at(name);
        const auto r = entries.at(a).new_name(regs, a);
        backref.emplace(r, a);
        return r;
    }

    void pop(const reg& name)
    {
        entries.at(backref.at(name)).pop();
    }

private:
    struct map_entry {
        map_entry(const reg& a) : counter{0}
        {
            stack.emplace(a);
        }
        reg current_name() const {return stack.top(); }
        reg new_name(regs_table& regs, const reg& a)
        {
            const auto n = regs.get(a) + "_" + std::to_string(counter++);
            const auto r = regs.put(n);
            stack.emplace(r);
            return current_name();
        }
        void pop() { stack.pop(); }
    private:
        std::stack<reg> stack;
        int counter;
    };
    std::map<reg, map_entry> entries;
    std::map<reg, reg> backref;
    regs_table& regs;
};

} // anonymous namespace

// algorithm is due to Cytron et al.
void subroutine::construct_minimal_ssa()
{
    // Initialization
    const auto variables = collect_variable_names();
    recompute_dominance();

    // Step 1: insert (incomplete) phi-functions
    insert_temp_phi(variables, *this);

    // Step 2: append indices to variable names
    name_manager names(this->regs, variables);
    std::function<void(basic_block&)> rename = [&] (basic_block& x)
    {
        for (auto& instr : x.instructions) {
            if (instr.type != instruction_type::PHI) {
                // Uses get current name
                instr.use_apply([&] (argument& a) {
                    if (a.is_reg()) {
                        a = names.current_name(a.get_reg());
                    }
                });
            }
            // Definitions get new name
            instr.def_apply([&] (argument& a) {
                a = names.new_name(a.get_reg());
            });
        }

        // Complete uses in temporary phi-function using current names
        for_each_cfg_successor(x.name, [&] (basic_block& y) {
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
            instr.def_apply([&] (argument& a) {
                names.pop(a.get_reg());
            });
        }
    };
    rename(entry_node());
}

}} // namespace hcc::ssa
