// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#include "interference_graph.h"
#include "ssa.h"

namespace hcc {
namespace ssa {
namespace {

using interference_graph_type = interference_graph<reg, reg>;

interference_graph_type build_interference_graph(subroutine& s, const std::vector<reg>& colors)
{
    interference_graph_type interference(s.collect_variable_names(), colors);

    s.recompute_liveness();
    s.for_each_bb([&](basic_block& block) {
        auto livenow = block.liveout;
        std::for_each(block.instructions.rbegin(), block.instructions.rend(), [&](instruction& i) {
            i.def_apply([&](argument& arg) {
                if (!arg.is_reg()) {
                    return;
                }
                const auto& x = arg.get_reg();
                for (const auto& y : livenow) {
                    interference.add_edge(x, y);
                }
                livenow.erase(x);
            });
            i.use_apply([&](argument& arg) {
                if (arg.is_reg()) {
                    livenow.insert(arg.get_reg());
                }
            });
        });
    });

    return interference;
}

void do_spill(subroutine& s, const reg& r)
{
    const argument storage = s.create_local();
    s.add_debug(storage, "spilled", r);

    s.for_each_bb([&](basic_block& bb) {
        for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e;) {
            const argument temporary = s.create_reg();
            s.add_debug(temporary, "spilled", r);
            bool did_spill_load = false;
            bool did_spill_store = false;

            auto& instruction = *i;
            instruction.use_apply([&](argument& arg) {
                if (!arg.is_reg()) {
                    return;
                }
                auto& x = arg.get_reg();
                if (x == r) {
                    arg = temporary;
                    did_spill_load = true;
                }
            });
            instruction.def_apply([&](argument& arg) {
                if (!arg.is_reg()) {
                    return;
                }
                auto& x = arg.get_reg();
                if (x == r) {
                    arg = temporary;
                    did_spill_store = true;
                }
            });
            if (did_spill_load) {
                bb.instructions.insert(
                    i, hcc::ssa::instruction(instruction_type::LOAD, {temporary, storage}));
            }
            if (did_spill_store) {
                if (instruction.type == instruction_type::MOV
                    && instruction.arguments[0] == temporary) {
                    instruction.type = instruction_type::STORE;
                    instruction.arguments[0] = storage;
                    ++i;
                } else {
                    ++i;
                    bb.instructions.insert(
                        i, hcc::ssa::instruction(instruction_type::STORE, {storage, temporary}));
                }
            } else {
                ++i;
            }
        }
    });
}

void do_allocate(subroutine& s, const interference_graph_type& interference)
{
    s.for_each_bb([&](basic_block& bb) {
        for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e;) {
            auto f = [&](argument& arg) {
                if (arg.is_reg()) {
                    arg = argument(interference.get_color(arg.get_reg()));
                }
            };
            i->use_apply(f);
            i->def_apply(f);
            if (i->type == instruction_type::MOV && i->arguments[0] == i->arguments[1]) {
                i = bb.instructions.erase(i);
            } else {
                ++i;
            }
        }
    });
}

} // namespace {

void subroutine::allocate_registers()
{
    std::vector<reg> colors;
    for (int i = 0; i < 7; ++i) {
        const auto r = create_reg();
        add_debug(r, "register");
        colors.push_back(r);
    }

    for (;;) {
        auto interference = build_interference_graph(*this, colors);
        const auto spilled = interference.find_coloring();
        if (spilled) {
            do_spill(*this, *spilled);
            construct_minimal_ssa();
            dead_code_elimination();
            copy_propagation();
            dead_code_elimination();
            ssa_deconstruct();
        } else {
            do_allocate(*this, interference);
            break;
        }
    }
}

} // namespace ssa {
} // namespace hcc {
