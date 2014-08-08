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
    const int node_count = s.g.node_count();
    int iteration = 0;
    std::vector<int> has_already(node_count, iteration);
    std::vector<int> work(node_count, iteration);

    std::set<int> w; // worklist of CFG nodes being processed
    for (const auto& variable : variables) {
        ++iteration;

        for (size_t block = 0; block < s.nodes.size(); ++block) {
            // is variable assigned in this block?
            bool assigned = false;
            for (auto& instruction : s.nodes[block]) {
                instruction.def_apply([&] (std::string& s) { assigned = assigned || (s == variable); });
            }

            // add to worklist
            if (assigned) {
                work[block] = iteration;
                w.emplace(block);
            }
        }

        while (!w.empty()) {
            int x = *w.begin();
            w.erase(w.begin());

            // for each y in dfs(x)
            for (int y : s.dominance->dfs[x]) {
                if (has_already[y] < iteration) {
                    // place PHI after LABEL
                    s.instructions.emplace(++s.nodes[y].begin(), instruction(instruction_type::PHI, {variable}));

                    has_already[y] = iteration;
                    if (work[y] < iteration) {
                        work[y] = iteration;
                        w.emplace(y);
                    }
                }
            }
        }
    }
}


/** Replace variables and complete phi functions */
class search_and_replace {
public:
    search_and_replace(const std::set<std::string>& variables)
    {
        for (const auto& v : variables) {
            C[v] = 0;
            S[v].push(0);
        }
    }

    void search(const int x, const subroutine& s)
    {
        std::vector<std::string> oldlhs;

        for (auto& instr : s.nodes[x]) {
            // rename uses
            if (instr.type != instruction_type::PHI) {
                instr.use_apply([&] (std::string& s) {
                    s = strip(s);
                    add_subscript(s, S.at(s).top());
                });
            }

            // rename definitions
            instr.def_apply([&] (std::string& s) {
                const std::string name = s;
                oldlhs.push_back(name);
                int i = C[name];
                add_subscript(s, i);
                S[name].push(i);
                ++C[name];
            });
        }

        // for each successor y of x
        for (int y : s.g.successors()[x]) {
            std::string whichpred = s.nodes.at(x).name;

            for (auto& instr : s.nodes[y]) {
                if (instr.type == instruction_type::PHI) {
                    instr.arguments.push_back(whichpred);
                    instr.arguments.push_back(strip(instr.arguments[0])); //TODO fugly
                    add_subscript(instr.arguments.back(), S.at(instr.arguments.back()).top());
                }
            }
        }

        // for each child y of x
        for (int i : s.dominance->tree.successors()[x]) {
            search(i, s);
        }

        // clean up
        for (auto& instr : s.nodes[x]) {
            instr.def_apply([&] (std::string& s) {
                S.at(strip(s)).pop();
            });
        }
    }

private:
    std::map<std::string, std::stack<int>> S;
    std::map<std::string, int> C;

    void add_subscript(std::string& s, int i) const
    {
        s += "_" + std::to_string(i);
    }

    std::string strip(std::string s) { return s.substr(0, s.rfind('_')); }
};

} // anonymous namespace

// algorithm is due to Cytron et al.
void subroutine::construct_minimal_ssa()
{
    // collect all the referenced variables
    std::set<std::string> variables;
    auto inserter = [&] (std::string& s) { variables.insert(s); };
    for (auto& instruction : instructions) {
        instruction.use_apply(inserter);
        instruction.def_apply(inserter);
    }

    recompute_control_flow_graph();
    insert_temp_phi(variables, *this);
    search_and_replace(variables).search(entry_node, *this);
    dead_code_elimination();
}

}} // namespace hcc::ssa