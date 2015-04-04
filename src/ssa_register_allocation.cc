// Copyright (c) 2013 Dano Pernis
// See LICENSE.txt

#include "ssa.h"

namespace hcc { namespace ssa {


const int color_unknown = -1;
const int color_spill = -2;

std::map<reg, int> color(
    const std::set<reg>& names,
    std::set<std::pair<reg, reg>> interference)
{
    // todo remove linear scans
    const int regs = 7;

    std::map<reg, int> color;
    for (const auto& name : names) {
        color[name] = color_unknown;
    }

    std::stack<reg> stack;
    auto interference_bak = interference;

    auto names_buffer = names;
    while (!names_buffer.empty()) {

        std::map<reg, int> counters;
        for (const auto& name : names) {
            counters[name] = 0;
        }
        for (const auto& xy : interference) {
            ++counters[xy.first];
            ++counters[xy.second];
        }

        reg remove = *names_buffer.begin();
        reg max = *names_buffer.begin();
        int max_count = -1;
        bool removed = false;
        for (const auto& name : names_buffer) {
            auto& count = counters.at(name);
            if (count > 0 && count < regs) {
                remove = name;
                removed = true;
            }
            if (count > max_count) {
                max_count = count;
                max = name;
            }
        }
        if (!removed) {
            remove = max;
        }
        stack.push(remove);

        for (auto& pair : interference) {
            if (pair.first == remove || pair.second == remove) {
                interference.erase(pair);
            }
        }
        names_buffer.erase(remove);
    }

    decltype(interference_bak) interference_new;
    while (!stack.empty()) {
        auto node = stack.top();
        stack.pop();
        // rebuild graph
        for (const auto& xy : interference_bak) {
            if (xy.first == node) {
                interference_new.insert(xy);
            }
            if (xy.second == node) {
                interference_new.insert(xy);
            }
        }

        // collect neighbours colors
        std::set<int> neighbour_colors;
        for (const auto& xy : interference_new) {
            if (xy.first == node) {
                neighbour_colors.insert(color.at(xy.second));
            }
            if (xy.second == node) {
                neighbour_colors.insert(color.at(xy.first));
            }
        }

        // color node
        color[node] = color_spill;
        for (int c = 0; c < regs; ++c) {
            if (neighbour_colors.count(c) == 0) {
                color[node] = c;
                break;
            }
        }
        if (color[node] == color_spill)
            break;
    }

    return color;
}


void subroutine::allocate_registers()
{
    for(;;) {
        recompute_liveness();

        // build interference graph
        std::set<std::pair<reg, reg>> interference;
        for_each_bb([&] (basic_block& block) {
            auto livenow = block.liveout;
            std::for_each(block.instructions.rbegin(), block.instructions.rend(), [&] (instruction& i) {
                i.def_apply([&] (argument& arg) {
                    if (!arg.is_reg()) {
                        return;
                    }
                    const auto& x = arg.get_reg();
                    for (const auto& y : livenow) {
                        if (!(x == y)) {
                            interference.emplace(
                                std::max(x, y),
                                std::min(x, y)
                            );
                        }
                    }
                    livenow.erase(x);
                });
                i.use_apply([&] (argument& arg) {
                    if (arg.is_reg()) {
                        livenow.insert(arg.get_reg());
                    }
                });
            });
        });

        // find a coloring
        const auto names = collect_variable_names();
        auto colors = color(names, interference);

        auto spilled = std::find_if(colors.begin(), colors.end(), [&] (const std::pair<reg, int>& kv) {
            return kv.second == color_spill;
        });
        if (spilled != colors.end()) {
            const auto& r = spilled->first;

            // spill
            int spill_counter = 0;
            for_each_bb([&] (basic_block& bb) {
            for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e;) {
                const auto x = regs.get(r);
                const argument spill_first = argument(locals.put("SPILLED_" + x));
                const argument spill_second = argument(regs.put(x + "_" + std::to_string(++spill_counter)));
                bool did_spill_load = false;
                bool did_spill_store = false;

                auto& instruction = *i;
                instruction.use_apply([&] (argument& arg) {
                    if (!arg.is_reg()) {
                        return;
                    }
                    auto& x = arg.get_reg();
                    if (x == r) {
                        arg = spill_second;
                        did_spill_load = true;
                    }
                });
                instruction.def_apply([&] (argument& arg) {
                    if (!arg.is_reg()) {
                        return;
                    }
                    auto& x = arg.get_reg();
                    if (x == r) {
                        arg = spill_second;
                        did_spill_store = true;
                    }
                });
                if (did_spill_load) {
                    bb.instructions.insert(i, hcc::ssa::instruction(instruction_type::LOAD, {spill_second, spill_first}));
                }
                // meh
                if (did_spill_store) {
                    if (instruction.type == instruction_type::MOV && instruction.arguments[0] == spill_second) {
                        instruction.type = instruction_type::STORE;
                        instruction.arguments[0] = spill_first;
                        ++i;
                    } else {
                        ++i;
                        bb.instructions.insert(i, hcc::ssa::instruction(instruction_type::STORE, {spill_first, spill_second}));
                    }
                } else {
                    ++i;
                }
            }
            });
        } else {
            for_each_bb([&] (basic_block& bb) {
            for (auto i = bb.instructions.begin(), e = bb.instructions.end(); i != e;) {
                auto& instruction = *i;

                auto f = [&] (argument& arg) {
                    if (arg.is_reg()) {
                        auto c = colors.at(arg.get_reg());
                        assert (c != color_unknown);
                        arg = argument(regs.put("R" + std::to_string(c)));
                    }
                };
                instruction.use_apply(f);
                instruction.def_apply(f);
                if (instruction.type == instruction_type::MOV && instruction.arguments[0] == instruction.arguments[1]) {
                    i = bb.instructions.erase(i);
                } else {
                    ++i;
                }
            }
            });
            return;
        }
    };
}

}} // namespace hcc::ssa
