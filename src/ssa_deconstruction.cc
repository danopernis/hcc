// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "ssa.h"

namespace hcc {
namespace ssa {
namespace {

struct congruence_classes {
    std::map<argument, reg> classes;

    std::function<void(argument&)> replacer()
    {
        return [&](argument& arg) {
            if (!arg.is_reg()) {
                return;
            }

            auto it = classes.find(arg);
            if (it != classes.end()) {
                arg = it->second;
            }
        };
    }
};

// Implements "Method I" from paper by Sreedhar et al.
// "Translating Out of Static Single Assignment Form"
// FIXME copies are not parallel
void naive_copy_insertion(subroutine& s, congruence_classes& cc)
{
    // worklist of MOV instructions to be inserted at the end of basic block,
    // indexed by basic block
    std::map<label, instruction_list> worklist;

    // pass 1: fill worklist and insert primed copy
    s.for_each_bb([&](basic_block& bb) {
        for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e; ++i) {
            if (i->type != instruction_type::PHI)
                continue;

            auto arg = i->arguments.begin();

            // invent a new primed name
            const auto base = s.create_reg();
            s.add_debug(base, "phi_dst_reg", *arg);

            // insert MOV after PHI
            bb.instructions.insert(++decltype(i)(i),
                                   instruction(instruction_type::MOV, {*arg, base}));
            cc.classes.emplace(base, base);
            cc.classes.emplace(*arg, base);

            // rename PHI's dest
            *arg++ = base;

            while (arg != i->arguments.end()) {
                auto label = *arg++;
                auto value = *arg;

                const auto name = s.create_reg();
                s.add_debug(name, "phi_src_reg", base);
                s.add_debug(name, "phi_src_label", label);

                // insert MOV into worklist
                worklist[label.get_label()].emplace_back(
                    instruction(instruction_type::MOV, {name, value}));
                cc.classes.emplace(name, base);

                // rename PHI's src
                *arg++ = name;
            }
        }
    });

    // pass 2: paste instructions from worklist at the end of respective basic block
    s.for_each_bb([&](basic_block& bb) {
        bb.instructions.splice(--bb.instructions.end(), worklist[bb.name]);
    });
}

// Determine if two registers *a* and *b* interfere in subroutine *s*
//
// Interfering registers have intersecting live ranges and different values:
//
// live(x) ... set of statements where x is live
// live(a) intersects live(b) ... def(a) in live(b) or def(b) in live(a)
// V(x) ... V(b) = V(a) if b <- a
//          V(b) = b    otherwise
// a interfere b ... live(a) intersects live(b) and V(a) != V(b)
bool interfere(const reg& a, const reg& b, subroutine& s)
{
    bool def_a_in_live_b = false;
    bool def_b_in_live_a = false;
    bool live_a = false;
    bool live_b = false;

    // At this point, code is still in SSA, so value of each register
    // is determined solely by its defining instruction. What is more,
    // we can use pointer here because nothing gets reallocated.
    // TODO use const *s* to prove that pointer usage is indeed safe.
    std::map<reg, const instruction*> value;

    // TODO investigate if domtree is really needed
    // TODO live ranges are probably overestimated at the end

    s.for_each_bb_in_domtree_preorder([&](basic_block& block) {
        for (auto& instr : block.instructions) {
            // live ranges
            instr.def_apply([&](argument& def) {
                assert (def.is_reg());
                if (def.get_reg() == a) {
                    live_a = true;
                    if (live_b)
                        def_a_in_live_b = true;
                } else if (def == b) {
                    live_b = true;
                    if (live_a)
                        def_b_in_live_a = true;
                }
            });

            // values
            if (instr.type == instruction_type::MOV) {
                if (instr.arguments[1].is_reg()) {
                    value.emplace(instr.arguments[0].get_reg(),
                        value.at(instr.arguments[1].get_reg()));
                } else {
                    value.emplace(instr.arguments[0].get_reg(), &instr);
                }
            } else {
                instr.def_apply([&](argument& arg) {
                    assert (arg.is_reg());
                    value.emplace(arg.get_reg(), &instr);
                });
            }
        }
    });

    const bool intersect = def_a_in_live_b || def_b_in_live_a;
    const bool differ_in_value = value.at(a) != value.at(b);
    return intersect && differ_in_value;
}

} // namespace {

// Inspired by paper
// "Revisiting Out-of-SSA Translation for Correctness, Code Quality, and Efficiency"
void subroutine::ssa_deconstruct()
{
    recompute_dominance();

    congruence_classes cc;
    naive_copy_insertion(*this, cc);

    // incidental classes, rising from the code
    for_each_bb([&](basic_block& bb) {
        for (auto& instr : bb.instructions) {
            if (instr.type == instruction_type::MOV) {
                auto& dest = instr.arguments[0];
                auto& src = instr.arguments[1];
                const bool has_src_class = cc.classes.count(src) > 0;
                const bool has_dest_class = cc.classes.count(dest) > 0;
                if (has_src_class && has_dest_class && cc.classes.at(src) == cc.classes.at(dest))
                    continue;

                if (src.is_reg() && dest.is_reg()) {
                    if (!interfere(src.get_reg(), dest.get_reg(), *this)) {
                        if (!has_src_class && !has_dest_class) {
                            const auto r = create_reg();
                            add_debug(r, "congruence_class");
                            cc.classes.emplace(src, r);
                            cc.classes.emplace(dest, r);
                        } else if (has_src_class && has_dest_class) {
                            auto keep_class = cc.classes.at(src);
                            auto remove_class = cc.classes.at(dest);
                            for (auto& kv : cc.classes) {
                                if (kv.second == remove_class)
                                    kv.second = keep_class;
                            }
                        }
                    }
                }
            }
        }
    });

    // pass 2: remove phis, replace names, remove nop moves
    // FIXME materialize parallel moves here, do not add them in the first place
    for_each_bb([&](basic_block& bb) {
        for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e;) {
            if (i->type == instruction_type::PHI) {
                i = bb.instructions.erase(i);
            } else {
                i->def_apply(cc.replacer());
                i->use_apply(cc.replacer());
                if (i->type == instruction_type::MOV && i->arguments[0] == i->arguments[1]) {
                    i = bb.instructions.erase(i);
                } else {
                    ++i;
                }
            }
        }
    });
}

} // namespace ssa {
} // namespace hcc {
