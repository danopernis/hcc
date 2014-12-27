// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <sstream>


namespace hcc { namespace ssa {


namespace {


struct congruence_classes {
    int last_class = 0;
    std::map<std::string, int> classes;

    void insert(std::string s) {
        classes.emplace(std::move(s), last_class);
    }

    std::function<void(std::string&)> replacer() {
        return [&](std::string& s) {
            auto it = classes.find(s);
            if (it != classes.end()) {
                s = "%class" + std::to_string(it->second);
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
    std::map<std::string, instruction_list> worklist;

    // pass 1: fill worklist and insert primed copy
    s.for_each_bb([&] (basic_block& bb) {
    for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e; ++i) {
        if (i->type != instruction_type::PHI)
            continue;

        auto arg = i->arguments.begin();

        // invent a new primed name
        const std::string base = *arg + "'";

        // insert MOV after PHI
        bb.instructions.insert(++decltype(i)(i), instruction(instruction_type::MOV, {*arg, base}));
        cc.insert(*arg);
        cc.insert(base);

        // rename PHI's dest
        *arg++ = base;

        while (arg != i->arguments.end()) {
            auto label = *arg++;
            auto value = *arg;

            const std::string name = base + label; // unique name

            // insert MOV into worklist
            worklist[label].emplace_back(instruction_type::MOV, std::vector<std::string>({name, value}));
            cc.insert(name);

            // rename PHI's src
            *arg++ = name;
        }
        ++cc.last_class;
    }
    });

    // pass 2: paste instructions from worklist at the end of respective basic block
    s.for_each_bb([&] (basic_block& bb) {
        bb.instructions.splice(--bb.instructions.end(), worklist[bb.name]);
    });
}


// live(x) ... set of statements were x is live
// live(a) intersects live(b) ... def(a) in live(b) or def(b) in live(a)
// V(x) ... V(b) = V(a) if b <- a
//          V(b) = b    otherwise
// a interfere b ... live(a) intersects live(b) and V(a) != V(b)
bool interfere(std::string a, std::string b, subroutine& s)
{
    bool def_a_in_live_b = false;
    bool def_b_in_live_a = false;
    bool live_a = false;
    bool live_b = false;
    std::map<std::string, std::string> V; // value of variable

    // TODO investigate if domtree is really needed
    // TODO live ranges are probably overestimated at the end

    s.for_each_bb_in_domtree_preorder([&] (basic_block& block) {
        for (auto& instr: block.instructions) {
            // live ranges
            instr.def_apply([&] (std::string& def) {
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
                V[instr.arguments[0]] = V[instr.arguments[1]];
            } else {
                instr.def_apply([&] (std::string& s) {
                    std::stringstream ss;
                    ss << instr;
                    V[s] = ss.str();
                });
            }
        }
    });

    const bool intersect = def_a_in_live_b || def_b_in_live_a;
    const bool differ_in_value = V.at(a) != V.at(b);
    return intersect && differ_in_value;
}


} // anonymous namespace


// Inspired by paper
// "Revisiting Out-of-SSA Translation for Correctness, Code Quality, and Efficiency"
void subroutine::ssa_deconstruct()
{
    recompute_control_flow_graph();

    congruence_classes cc;
    naive_copy_insertion(*this, cc);

    // incidental classes, rising from the code
    for_each_bb([&] (basic_block& bb) {
    for (auto& instr: bb.instructions) {
        if (instr.type == instruction_type::MOV) {
            auto &dest = instr.arguments[0];
            auto &src = instr.arguments[1];
            const bool has_src_class =  cc.classes.count(src) > 0;
            const bool has_dest_class = cc.classes.count(dest) > 0;
            if (has_src_class && has_dest_class && cc.classes.at(src) == cc.classes.at(dest))
                continue;

            if (!src.empty() && src[0] == '%' && !dest.empty() && dest[0] == '%') {
                if (!interfere(src, dest, *this)) {
                    if (!has_src_class && !has_dest_class) {
                        cc.classes.emplace(src, cc.last_class);
                        cc.classes.emplace(dest, cc.last_class);
                        ++cc.last_class;
                    } else if (has_src_class && has_dest_class) {
                        int keep_class = cc.classes.at(src);
                        int remove_class = cc.classes.at(dest);
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
    for_each_bb([&] (basic_block& bb) {
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

}} // namespace hcc::ssa
