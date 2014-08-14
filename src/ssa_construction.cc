// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <set>
#include <string>

namespace hcc { namespace ssa {

namespace {

/** Use dominance frontier set to find where phi-functions are needed */
void insert_temp_phi(
    const std::set<std::string>& variables,
    subroutine& s)
{
    // init
    int iteration = 0;
    for (auto& block : s.nodes) {
        block.has_already = iteration;
        block.work = iteration;
    }

    std::set<int> w; // worklist of CFG nodes being processed
    for (const auto& variable : variables) {
        ++iteration;

        for (auto& block : s.nodes) {
            for (auto& instruction : block) {
                instruction.def_apply([&] (std::string& def) {
                    if (def == variable) {
                        // add to worklist
                        block.work = iteration;
                        w.emplace(block.index);
                    }
                });
            }
        }

        while (!w.empty()) {
            int x = *w.begin();
            w.erase(w.begin());

            // for each y in dfs(x)
            for (int y : s.dominance->dfs[x]) {
                auto& block = s.nodes[y];
                if (block.has_already < iteration) {
                    // place PHI after LABEL
                    s.instructions.emplace(++block.begin(), instruction(instruction_type::PHI, {variable}));

                    block.has_already = iteration;
                    if (block.work < iteration) {
                        block.work = iteration;
                        w.emplace(block.index);
                    }
                }
            }
        }
    }
}

struct name_manager {
    name_manager(const std::set<std::string>& variables)
    {
        for (const auto& v : variables) {
            counter[v] = 0;
            stack[v].push(0);
        }
    }

    void rename_to_current(std::string& name)
    {
        name += "_" + std::to_string(stack[name].top());
    }

    void rename_to_new(std::string& name)
    {
        stack[name].push(counter[name]++);
        rename_to_current(name);
    }

    void pop(const std::string& name)
    {
        stack.at(strip(name)).pop();
    }

    std::string strip(std::string name) const
    {
        return name.substr(0, name.rfind('_'));
    }

private:
    std::map<std::string, std::stack<int>> stack;
    std::map<std::string, int> counter;
};

} // anonymous namespace

// algorithm is due to Cytron et al.
void subroutine::construct_minimal_ssa()
{
    // Initialization
    const auto variables = collect_variable_names();
    recompute_control_flow_graph();

    // Step 1: insert (incomplete) phi-functions
    insert_temp_phi(variables, *this);

    // Step 2: append indices to variable names
    name_manager names(variables);
    std::function<void(basic_block&)> rename = [&] (basic_block& x)
    {
        // Rewrite variable names according to this table:
        //
        //         |     use           def
        // --------+--------------------------
        // regular | current name    new name
        // phi     |     skip        new name
        //
        for (auto& instr : x) {
            if (instr.type != instruction_type::PHI) {
                instr.use_apply([&] (std::string& def) { names.rename_to_current(def); });
            }
            instr.def_apply([&] (std::string& def) { names.rename_to_new(def); });
        }

        // Complete uses in temporary phi-function using current names
        for_each_cfg_successor(x.index, [&] (basic_block& y) {
            for (auto& instr : y) {
                if (instr.type == instruction_type::PHI) {
                    instr.arguments.push_back(x.name);
                    instr.arguments.push_back(names.strip(instr.arguments[0]));
                    names.rename_to_current(instr.arguments.back());
                }
            }
        });

        // Recursive call
        for_each_domtree_successor(x.index, rename);

        // Cleanup
        for (auto& instr : x) {
            instr.def_apply([&] (std::string& def) { names.pop(def); });
        }
    };
    rename(entry_node());
}

}} // namespace hcc::ssa
