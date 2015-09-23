// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#ifndef SSA_H
#define SSA_H

#include "graph.h"
#include "graph_dominance.h"
#include "index_table.h"
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

namespace hcc {

struct asm_program;

namespace jack {
namespace ast {

struct Class;

} // namespace ast
} // namespace jack

namespace ssa {


struct unit;
struct subroutine_ir;;

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

// ============================================================================

enum class argument_type {
    CONSTANT,
    REG,
    GLOBAL,
    LOCAL,
    LABEL,
};

struct constant {
    constant(int value) : value(value) { }
    int value;
    bool operator< (const constant& other) const { return value <  other.value; }
    bool operator==(const constant& other) const { return value == other.value; }
    std::string save_fast() const;
};

// ============================================================================

struct reg {
    bool operator< (const reg& other) const { return index <  other.index; }
    bool operator==(const reg& other) const { return index == other.index; }
    std::string save_fast() const;
private:
    reg(int index) : index(index) { }
    int index;
friend struct subroutine_ir;
};

// ============================================================================

struct global {
    bool operator< (const global& other) const { return index <  other.index; }
    bool operator==(const global& other) const { return index == other.index; }
    std::string save_fast() const;
private:
    global(int index) : index(index) { }
    int index; // index to the compilation unit's table of globals
friend struct globals_table;
};

struct globals_table : index_table<globals_table, std::string, global> {
    static global construct(int index) { return global(index); }
};

// ============================================================================

struct local {
    bool operator< (const local& other) const { return index <  other.index; }
    bool operator==(const local& other) const { return index == other.index; }
    std::string save_fast() const;
private:
    local(int index) : index(index) { }
    int index;
friend struct subroutine_ir;
};

// ============================================================================

struct label {
    bool operator< (const label& other) const { return index <  other.index; }
    bool operator==(const label& other) const { return index == other.index; }
    std::string save_fast() const;
private:
    label(int index) : index(index) { }
    int index;
friend struct subroutine_ir;
};

// ============================================================================

struct argument {

    argument(const constant& v) : type(argument_type::CONSTANT), value(v) { }
    argument(const reg&      v) : type(argument_type::REG     ), value(v) { }
    argument(const global&   v) : type(argument_type::GLOBAL  ), value(v) { }
    argument(const local&    v) : type(argument_type::LOCAL   ), value(v) { }
    argument(const label&    v) : type(argument_type::LABEL   ), value(v) { }

    bool is_constant() const { return type == argument_type::CONSTANT; }
    bool is_reg()      const { return type == argument_type::REG;      }
    bool is_global()   const { return type == argument_type::GLOBAL;   }
    bool is_local()    const { return type == argument_type::LOCAL;    }
    bool is_label()    const { return type == argument_type::LABEL;    }

    const constant& get_constant() const { assert(is_constant()); return value.constant_value; }
    const reg&      get_reg()      const { assert(is_reg());      return value.reg_value;      }
    const global&   get_global()   const { assert(is_global());   return value.global_value;   }
    const local&    get_local()    const { assert(is_local());    return value.local_value;    }
    const label&    get_label()    const { assert(is_label());    return value.label_value;    }

    void save(std::ostream&, unit&, subroutine_ir&) const;
    std::string save_fast() const;

private:
    argument() {}

    argument_type type;
    union value {
        value() { }
        value(const constant& v) : constant_value(v) { }
        value(const reg&      v) : reg_value     (v) { }
        value(const global&   v) : global_value  (v) { }
        value(const local&    v) : local_value   (v) { }
        value(const label&    v) : label_value   (v) { }

        constant constant_value;
        reg reg_value;
        global global_value;
        local local_value;
        label label_value;
    } value;

friend bool operator<(const argument& a, const argument& b);
friend bool operator==(const argument& a, const argument& b);
};
inline bool operator<(const argument& a, const argument& b)
{
    if (a.is_constant() && b.is_constant()) {
        return a.get_constant() < b.get_constant();
    }
    if (a.is_reg() && b.is_reg()) {
        return a.get_reg() < b.get_reg();
    }
    if (a.is_global() && b.is_global()) {
        return a.get_global() < b.get_global();
    }
    if (a.is_local() && b.is_local()) {
        return a.get_local() < b.get_local();
    }
    if (a.is_label() && b.is_label()) {
        return a.get_label() < b.get_label();
    }
    return a.type < b.type;
}
inline bool operator==(const argument& a, const argument& b)
{
    if (a.is_constant() && b.is_constant()) {
        return a.get_constant() == b.get_constant();
    }
    if (a.is_reg() && b.is_reg()) {
        return a.get_reg() == b.get_reg();
    }
    if (a.is_global() && b.is_global()) {
        return a.get_global() == b.get_global();
    }
    if (a.is_local() && b.is_local()) {
        return a.get_local() == b.get_local();
    }
    if (a.is_label() && b.is_label()) {
        return a.get_label() == b.get_label();
    }
    return a.type == b.type;
}

/** Represents an atomic SSA instruction. */
struct instruction {
    instruction(instruction_type type, std::vector<argument> arguments)
        : type{type}
        , arguments{std::move(arguments)}
    {}

    instruction_type type;
    std::vector<argument> arguments;

    // see subroutine::dead_code_elimination()
    int work;

    void use_apply(std::function<void(argument&)>);

    template<typename F>
    void def_apply(F&& f)
    {
        switch (type) {
        case instruction_type::JUMP:
        case instruction_type::JLT:
        case instruction_type::JEQ:
        case instruction_type::RETURN:
        case instruction_type::STORE:
            break;
        case instruction_type::CALL:
        case instruction_type::LOAD:
        case instruction_type::MOV:
        case instruction_type::NEG:
        case instruction_type::NOT:
        case instruction_type::ADD:
        case instruction_type::SUB:
        case instruction_type::AND:
        case instruction_type::OR:
        case instruction_type::PHI:
        case instruction_type::ARGUMENT:
            assert(arguments[0].is_reg());
            f(arguments[0]);
            break;
        default:
            assert(false);
        }
    }

    bool operator<(const instruction& other) const
    {
        if (type == other.type)
           return arguments < other.arguments;
        else
           return type < other.type;
    }

    void save(std::ostream&, unit&, subroutine_ir&) const;
    std::string save_fast() const;
};

using instruction_list = std::list<instruction>;

struct basic_block {
    basic_block(const label& name) : name(name) { }

    instruction_list instructions;

    label name;

    std::set<reg> uevar;
    std::set<reg> varkill;
    std::set<reg> liveout;

    // subroutine::construct_minimal_ssa()
    int work;
    int has_already;
};

/** Intermediate representation */
struct subroutine_ir {
    subroutine_ir()
        : exit_node_  {create_label()}
        , entry_node_ {create_label()}
    { }

    template<typename F>
    void for_each_domtree_successor(const label& l, F&& f)
    {
        for (int i : dominance->tree.successors()[l.index]) {
            f(basic_blocks.at({i}));
        }
    }

    template<typename F>
    void for_each_cfg_successor(const label& l, F&& f)
    {
        for (int i : g.successors()[l.index]) {
            f(basic_blocks.at({i}));
        }
    }

    template<typename F>
    void for_each_reverse_dfs(const label& l, F&& f)
    {
        for (int i : reverse_dominance->dfs[l.index]) {
            f(basic_blocks.at({i}));
        }
    }

    template<typename F>
    void for_each_bb_in_dfs(const label& l, F&& f)
    {
        for (int i : dominance->dfs[l.index]) {
            f(basic_blocks.at({i}));
        }
    }

    template<typename F>
    void for_each_bb_in_domtree_preorder(F&& f)
    {
        depth_first_search dfs(dominance->tree.successors(), dominance->root);
        for (int i : dfs.preorder()) {
            f(basic_blocks.at({i}));
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
    std::set<reg> collect_variable_names();

    basic_block& entry_node()
    { return basic_blocks.at(entry_node_); }

    basic_block& exit_node()
    { return basic_blocks.at(exit_node_); }

    unit& get_unit() {return *u;}
    unit* u;

    void add_edge(const label& from, const label& to)
    { g.add_edge(from.index, to.index); }

    label create_label()
    {
        assert(g.node_count() == label_counter);
        g.add_node();
        label l {label_counter++};
        basic_blocks.emplace(l, l);
        return l;
    }
    local create_local() { return {local_counter++}; }
    reg create_reg()     { return {reg_counter++  }; }
    template<typename... T> void add_debug(const argument& a, T&&... t) { /* TODO */ }

    std::map<label, basic_block> basic_blocks;
private:
    graph g;

    int label_counter {0};
    int local_counter {0};
    int reg_counter   {0};

    label exit_node_;
    label entry_node_;
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
    void superblock_clone();
    void sccp();
};

using subroutine_map = std::map<global, subroutine>;

struct unit {
    subroutine_map subroutines;
    globals_table globals;

    subroutine_map::iterator insert_subroutine(const global& name)
    {
        auto it = subroutines.find(name);
        if (it != subroutines.end()) {
            throw std::runtime_error("unit::insert_subroutine");
        } else {
            auto res = subroutines.insert(it, std::make_pair(name, subroutine()));
            res->second.u = this;
            return res;
        }
    }

    void load(std::istream&);
    void save(std::ostream&);
    void translate_to_asm(hcc::asm_program&);
    void translate_from_jack(const hcc::jack::ast::Class&);
};

}} // namespace hcc::ssa

#endif // SSA_H
