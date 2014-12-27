// Copyright (c) 2013 Dano Pernis
// See LICENSE for details

#ifndef SSA_H
#define SSA_H

#include "ssa.h"
#include "graph.h"
#include "graph_dominance.h"
#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <stack>
#include <list>
#include <functional>

namespace hcc { namespace ssa {


enum class instruction_type {
    ARGUMENT,
    BRANCH,
    JUMP,
    LABEL,
    CALL,
    RETURN,
    LOAD,
    STORE,
    MOV,
    PHI,
    ADD,
    SUB,
    AND,
    OR,
    LT,
    GT,
    EQ,
    NEG,
    NOT,
};


/** Represents an atomic SSA instruction. */
struct instruction {
    instruction(instruction_type type, std::vector<std::string> arguments)
        : type{type}
        , arguments{std::move(arguments)}
    {}

    instruction_type type;
    std::vector<std::string> arguments;

    // see subroutine::dead_code_elimination()
    bool mark;

    // see subroutine_ir::recompute_control_flow_graph()
    int basic_block;

    void use_apply(std::function<void(std::string&)>);
    void def_apply(std::function<void(std::string&)>);
    void label_apply(std::function<void(std::string&)>);

    bool operator<(const instruction& other) const
    {
        if (type == other.type)
           return arguments < other.arguments;
        else
           return type < other.type;
    }
};
std::ostream& operator<<(std::ostream& os, const instruction& instr);

using instruction_list = std::list<instruction>;

struct subroutine_ir;

// Represents a range of instructions
// Iterators are inclusive on construction; however end() iterator points just
// behind the range.
struct basic_block {
    basic_block(subroutine_ir& s) : instructions(s) { }

    struct proxy {
        proxy(subroutine_ir& s) : s(s) { }

        instruction_list::iterator begin() const
        { return first; }

        instruction_list::iterator end() const
        { return ++instruction_list::iterator(last); }

        template<typename F>
        void for_each_reverse(F&& f)
        {
            for (auto i = last, e = first; i != e; --i) {
                f(*i);
            }
        }

        instruction_list::iterator insert(instruction_list::iterator, const instruction&);
        instruction_list::iterator erase(instruction_list::iterator);
        void splice(instruction_list::iterator, instruction_list&);

    private:
        instruction_list::iterator first;
        instruction_list::iterator last;
        subroutine_ir& s;

    friend class subroutine_ir;
    };

    proxy instructions;

    std::string name;
    int index;

    std::set<std::string> uevar;
    std::set<std::string> varkill;
    std::set<std::string> liveout;
    std::set<int> successors;

    // subroutine::construct_minimal_ssa()
    int work;
    int has_already;
};

/** Intermediate representation */
struct subroutine_ir {
    template<typename F>
    void for_each_domtree_successor(int index, F&& f)
    {
        for (int i : dominance->tree.successors()[index]) {
            f(nodes[i]);
        }
    }

    template<typename F>
    void for_each_cfg_successor(int index, F&& f)
    {
        for (int i : nodes[index].successors) {
            f(nodes[i]);
        }
    }

    template<typename F>
    void for_each_reverse_dfs(int index, F&& f)
    {
        for (int i : reverse_dominance->dfs[index]) {
            f(nodes[i]);
        }
    }

    template<typename F>
    void for_each_bb_in_dfs(int index, F&& f)
    {
        for (int i : dominance->dfs[index]) {
            f(nodes[i]);
        }
    }

    template<typename F>
    void for_each_bb_in_domtree_preorder(F&& f)
    {
        depth_first_search dfs(dominance->tree.successors(), dominance->root);
        for (int i : dfs.preorder()) {
            f(nodes[i]);
        }
    }

    template<typename F>
    void for_each_bb(F&& f)
    {
        for (auto& block : nodes) {
            f(block);
        }
    }

    void recompute_control_flow_graph();
    void recompute_liveness();
    std::set<std::string> collect_variable_names();

    basic_block& entry_node()
    { return nodes[entry_node_]; }

private:
    graph g;
    int exit_node;
    int entry_node_;
    instruction_list exit_node_instructions;
    std::vector<basic_block> nodes;
    std::unique_ptr<graph_dominance> reverse_dominance;
    std::unique_ptr<graph_dominance> dominance;
    instruction_list instructions;

friend struct basic_block::proxy;
friend struct unit;
};

/** Transformations */
struct subroutine : public subroutine_ir {
    void construct_minimal_ssa();
    void dead_code_elimination();
    void copy_propagation();
    void ssa_deconstruct();
    void allocate_registers();
    void prettify_names(unsigned& var_counter, unsigned& label_counter);
};

using subroutine_map = std::map<std::string, subroutine>;

struct unit {
    subroutine_map subroutines;
    std::set<std::string> globals;

    subroutine_map::iterator insert_subroutine(
        const std::string& name,
        const instruction_list& instructions = instruction_list())
    {
        auto it = subroutines.find(name);
        if (it != subroutines.end()) {
            throw std::runtime_error("unit::insert_subroutine");
        } else {
            subroutine s;
            s.instructions = instructions;
            return subroutines.insert(it, std::make_pair(name, std::move(s)));
        }
    }

    void load(std::istream&);
    void save(std::ostream&) const;
};

}} // namespace hcc::ssa

#endif // SSA_H
