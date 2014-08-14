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
    bool mark;

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

// Represents a range of instructions
// Iterators are inclusive on construction; however end() iterator points just
// behind the range.
struct basic_block {
    instruction_list::iterator begin() const
    { return first; }

    instruction_list::iterator end() const
    { return ++instruction_list::iterator(last); }

    std::string name;
    int index;

    std::set<std::string> uevar;
    std::set<std::string> varkill;
    std::set<std::string> liveout;
    std::set<int> successors;

    // subroutine::construct_minimal_ssa()
    int work;
    int has_already;

//private:
    instruction_list::iterator first;
    instruction_list::iterator last;

friend class subroutine_ir;
};

/** Intermediate representation */
struct subroutine_ir {
    // TODO privatise these variables
    instruction_list instructions;
    std::vector<basic_block> nodes;
    std::unique_ptr<graph_dominance> dominance;
    int entry_node;

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
    void for_each_dfs(int index, F&& f)
    {
        for (int i : reverse_dominance->dfs[index]) {
            f(nodes[i]);
        }
    }

    void recompute_control_flow_graph();
    void recompute_liveness();
    std::set<std::string> collect_variable_names();

private:
    graph g;
    int exit_node;
    instruction_list exit_node_instructions;
    std::unique_ptr<graph_dominance> reverse_dominance;
};

/** Transformations */
struct subroutine : public subroutine_ir {
    void construct_minimal_ssa();
    void dead_code_elimination();
    void copy_propagation();
    void ssa_deconstruct();
    void allocate_registers();
    void prettify_names(unsigned& var_counter, unsigned& label_counter);
    void clean_cfg();
};

using subroutine_map = std::map<std::string, subroutine>;

struct unit {
    subroutine_map subroutines;
    std::set<std::string> globals;

    subroutine_map::iterator insert_subroutine(const std::string& name)
    {
        auto it = subroutines.find(name);
        if (it != subroutines.end())
            throw std::runtime_error("unit::insert_subroutine");
        else
            return subroutines.insert(it, std::make_pair(name, subroutine()));
    }

    void load(std::istream&);
    void save(std::ostream&) const;
};

}} // namespace hcc::ssa

#endif // SSA_H
