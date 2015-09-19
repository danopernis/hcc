// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <cassert>

namespace {

using hcc::ssa::argument;
using hcc::ssa::basic_block;
using hcc::ssa::instruction;
using hcc::ssa::instruction_type;
using hcc::ssa::label;
using hcc::ssa::reg;
using hcc::ssa::subroutine;

template<typename F>
void jump_label_apply(instruction& i, F&& f)
{
    switch (i.type) {
    case instruction_type::JEQ:
    case instruction_type::JLT:
        f(i.arguments[2]);
        f(i.arguments[3]);
        break;
    case instruction_type::JUMP:
        f(i.arguments[0]);
        break;
    default:
        break;
    }
}

struct context {
    argument replace(const argument& a) const
    {
        if (a.is_reg()) {
            return reg_table.at(a.get_reg());
        } else if (a.is_constant()) {
            return a;
        }
        assert (false);
    }

    bool ever_seen(const label& l) const
    { return std::find(begin(history), end(history), l) != end(history); }

    bool just_seen(const label& l) const
    { return !history.empty() && history.back() == l; }

    std::map<reg, reg> reg_table;
    std::map<label, label> label_table;
    std::vector<label> history;
};

struct cloning_algorithm {
    cloning_algorithm(subroutine& old_subroutine_, subroutine& new_subroutine_)
        : old_subroutine(old_subroutine_)
        , new_subroutine(new_subroutine_)
    { clone(old_subroutine.entry_node(), new_subroutine.entry_node(), {}); }

private:
    void clone(const basic_block& bb, basic_block& cbb, context ctx)
    {
        ctx.label_table.emplace(bb.name, cbb.name);
        const auto& old_instructions = bb.instructions;
        auto& new_instructions = cbb.instructions;

        for (auto i : old_instructions) {

            // each definition gets cloned
            i.def_apply([this,&ctx] (argument& a) {
                const auto r = a.get_reg();
                const auto cr = new_subroutine.create_reg();
                a = cr;
                ctx.reg_table.emplace(r, cr);
            });

            if (i.type == instruction_type::PHI) {
                start_phi(i, ctx);
            } else {
                i.use_apply([&ctx] (argument& a) {
                    a = ctx.replace(a);
                });
            }

            // this is needed for reverse dominance computation
            if (i.type == instruction_type::RETURN) {
                new_subroutine.add_edge(cbb.name, new_subroutine.exit_node().name);
            }

            jump_label_apply(i, [&] (argument& a) {
                const auto l = a.get_label();
                const auto is_back_edge = ctx.ever_seen(l);
                const auto cl = is_back_edge ? ctx.label_table.at(l) : new_subroutine.create_label();

                a = cl;
                new_subroutine.add_edge(cbb.name, cl);

                // recurrsion
                if (is_back_edge) {
                    finish_phi(bb.name, cbb.name, ctx);
                } else {
                    auto ctx2 = ctx;
                    ctx2.history.push_back(bb.name);
                    clone(old_subroutine.basic_blocks.at(l), new_subroutine.basic_blocks.at(cl), ctx2);
                }
            });

            new_instructions.push_back(i);
        }

        for (auto& i : new_instructions) {
            if (i.type == instruction_type::PHI) {
                rewrite_phi(i);
            }
        }
    }

    void start_phi(const instruction& old_phi, const context& ctx)
    {
        auto first = begin(old_phi.arguments);
        auto last  = end  (old_phi.arguments);

        auto old_def = (*first++).get_reg();
        auto new_def = old_def; // it is already translated
        instruction new_phi {instruction_type::PHI, {new_def}};

        while (first != last) {
            const auto& old_origin = (*first++).get_label();
            const auto& old_use    = *first++;
            if (ctx.just_seen(old_origin)) {
                new_phi.arguments.emplace_back(ctx.label_table.at(old_origin));
                new_phi.arguments.emplace_back(ctx.replace(old_use));
            } else if (ctx.label_table.count(old_origin) == 0) {
                phi_worklist.emplace_back(new_def, old_origin, old_use);
            }
        }
        phi_result.emplace(new_def, new_phi);
    }

    void finish_phi(const label& old_label, const label& new_label, const context& ctx)
    {
        for (auto& w : phi_worklist) {
            if (w.origin == old_label) {
                auto& arguments = phi_result.at(w.def).arguments;
                arguments.push_back(new_label);
                arguments.push_back(ctx.replace(w.use));
            }
        }
    }

    void rewrite_phi(instruction& i) const
    {
        i = phi_result.at(i.arguments[0].get_reg());
        if (i.arguments.size() == 3) {
            i = {instruction_type::MOV, {i.arguments[0], i.arguments[2]}};
        }
    }

    struct phi_worklist_entry {
        phi_worklist_entry(reg def_, label origin_, argument use_)
            : def {def_}, origin {origin_}, use {use_}
        { }

        reg def;
        label origin;
        argument use;
    };

    subroutine& old_subroutine;
    subroutine& new_subroutine;
    std::vector<phi_worklist_entry> phi_worklist;
    std::map<reg, instruction> phi_result;
};

} // anonymous namespace

namespace hcc { namespace ssa {

void subroutine::superblock_clone()
{
    subroutine result;
    cloning_algorithm a {*this, result};
    *this = std::move(result);
}

}} // end namespace hcc::ssa
