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

// live(x) ... set of statements were x is live
// live(a) intersects live(b) ... def(a) in live(b) or def(b) in live(a)
// V(x) ... V(b) = V(a) if b <- a
//          V(b) = b    otherwise
// a interfere b ... live(a) intersects live(b) and V(a) != V(b)
bool interfere(const argument& a, const argument& b, subroutine& s)
{
    assert(a.is_reg() && b.is_reg());

    bool def_a_in_live_b = false;
    bool def_b_in_live_a = false;
    bool live_a = false;
    bool live_b = false;
    std::map<std::string, std::string> V; // value of variable

    // TODO investigate if domtree is really needed
    // TODO live ranges are probably overestimated at the end

    s.for_each_bb_in_domtree_preorder([&](basic_block& block) {
        for (auto& instr : block.instructions) {
            // live ranges
            instr.def_apply([&](argument& def) {
                if (def == a) {
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
                auto s0 = instr.arguments[0].save_fast();
                auto s1 = instr.arguments[1].save_fast();
                V[std::move(s0)] = V[std::move(s1)];
            } else {
                instr.def_apply([&](argument& arg) {
                    auto s0 = arg.save_fast();
                    auto s1 = instr.save_fast();
                    V[std::move(s0)] = std::move(s1);
                });
            }
        }
    });

    const bool intersect = def_a_in_live_b || def_b_in_live_a;
    auto sa = a.save_fast();
    auto sb = b.save_fast();
    const bool differ_in_value = V.at(sa) != V.at(sb);
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
                    if (!interfere(src, dest, *this)) {
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
