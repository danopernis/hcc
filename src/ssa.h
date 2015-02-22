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
    JUMP,
    JLT,
    JEQ,
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

struct basic_block {
    instruction_list instructions;

    std::string name;
    int index;

    std::set<std::string> uevar;
    std::set<std::string> varkill;
    std::set<std::string> liveout;

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
            f(basic_blocks.at(i));
        }
    }

    template<typename F>
    void for_each_cfg_successor(int index, F&& f)
    {
        for (int i : g.successors()[index]) {
            f(basic_blocks.at(i));
        }
    }

    template<typename F>
    void for_each_reverse_dfs(int index, F&& f)
    {
        for (int i : reverse_dominance->dfs[index]) {
            f(basic_blocks.at(i));
        }
    }

    template<typename F>
    void for_each_bb_in_dfs(int index, F&& f)
    {
        for (int i : dominance->dfs[index]) {
            f(basic_blocks.at(i));
        }
    }

    template<typename F>
    void for_each_bb_in_domtree_preorder(F&& f)
    {
        depth_first_search dfs(dominance->tree.successors(), dominance->root);
        for (int i : dfs.preorder()) {
            f(basic_blocks.at(i));
        }
    }

    template<typename F>
    void for_each_bb(F&& f)
    {
        for (auto& block : basic_blocks) {
            f(block.second);
        }
    }

    template<typename F>
    void for_each_bb(F&& f) const { for_each_bb(std::forward<F>(f)); }

    void recompute_dominance();
    void recompute_liveness();
    std::set<std::string> collect_variable_names();

    basic_block& entry_node()
    { return basic_blocks.at(entry_node_); }

    basic_block& exit_node()
    { return basic_blocks.at(exit_node_); }

private:
    graph g;
    int exit_node_;
    int entry_node_;
    std::map<int, basic_block> basic_blocks;
    std::unique_ptr<graph_dominance> reverse_dominance;
    std::unique_ptr<graph_dominance> dominance;

friend struct subroutine_builder;
};

/** Transformations */
struct subroutine : public subroutine_ir {
    void construct_minimal_ssa();
    void dead_code_elimination();
    void copy_propagation();
    void ssa_deconstruct();
    void allocate_registers();
    void prettify_names(unsigned& var_counter);
};

using subroutine_map = std::map<std::string, subroutine>;

struct unit {
    subroutine_map subroutines;
    std::set<std::string> globals;

    subroutine_map::iterator insert_subroutine(const std::string& name)
    {
        auto it = subroutines.find(name);
        if (it != subroutines.end()) {
            throw std::runtime_error("unit::insert_subroutine");
        } else {
            return subroutines.insert(it, std::make_pair(name, subroutine()));
        }
    }

    void load(std::istream&);
    void save(std::ostream&);
};

}} // namespace hcc::ssa

#endif // SSA_H
