// Copyright (c) 2013 Dano Pernis
// See LICENSE.txt

#include "ssa.h"

namespace hcc { namespace ssa {


const int color_unknown = -1;
const int color_spill = -2;

std::map<std::string, int> color(
    const std::set<std::string>& names,
    std::set<std::pair<std::string, std::string>> interference)
{
    // todo remove linear scans
    const int regs = 7;

    std::map<std::string, int> color;
    for (const auto& name : names) {
        color[name] = color_unknown;
    }

    std::stack<std::string> stack;
    auto interference_bak = interference;

    auto names_buffer = names;
    while (!names_buffer.empty()) {

        std::map<std::string, int> counters;
        for (const auto& name : names) {
            counters[name] = 0;
        }
        for (const auto& xy : interference) {
            ++counters[xy.first];
            ++counters[xy.second];
        }

        std::string remove;
        std::string max;
        int max_count = -1;
        for (const auto& name : names_buffer) {
            auto& count = counters.at(name);
            if (count > 0 && count < regs) {
                remove = name;
            }
            if (count > max_count) {
                max_count = count;
                max = name;
            }
        }
        assert (!max.empty());
        if (remove.empty()) {
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
    bool did_spill;
    do {
        struct block_info {
            // TODO replace set with bitset
            std::set<std::string> uevar;
            std::set<std::string> varkill;
            std::set<std::string> liveout;
            std::set<std::string> successors;
            instruction_list::iterator first;
            instruction_list::iterator last;
        };
        std::map<std::string, block_info> blocks;
        std::map<std::string, block_info>::iterator current_block = blocks.end();

        // init
        for (auto i = instructions.begin(), e = instructions.end(); i != e; ++i) {
            auto& instruction = *i;
            if (instruction.type == instruction_type::LABEL) {
                current_block = blocks.emplace(instruction.arguments[0], block_info()).first;
                current_block->second.first = i;
            } else {
                auto& block = current_block->second;
                instruction.use_apply([&block] (const std::string& x) {
                    if (!block.varkill.count(x))
                        block.uevar.insert(x);
                });
                instruction.def_apply([&block] (const std::string& x) {
                    block.varkill.insert(x);
                });
                if (instruction.type == instruction_type::BRANCH ||
                    instruction.type == instruction_type::JUMP ||
                    instruction.type == instruction_type::RETURN)
                {
                    instruction.label_apply([&block] (const std::string& x) {
                        block.successors.insert(x);
                    });
                    block.last = i;
                }
            }
        }

        // find live ranges
        bool changed = true;
        while (changed) {
            changed = false;
            for (auto& kv : blocks) {
                auto& block = kv.second;
                auto old_liveout = block.liveout;

                std::set<std::string> new_liveout;
                for (const auto& successor: block.successors) {
                    const auto& uevar = blocks[successor].uevar;
                    const auto& varkill = blocks[successor].varkill;
                    const auto& liveout = blocks[successor].liveout;
                    std::copy(uevar.begin(), uevar.end(), std::inserter(new_liveout, new_liveout.begin()));
                    for (const auto& x : liveout) {
                        if (varkill.count(x) == 0)
                            new_liveout.insert(x);
                    }
                }
                block.liveout = new_liveout;

                if (block.liveout != old_liveout) {
                    changed = true;
                }
            }
        }

        // build interference graph
        std::set<std::pair<std::string, std::string>> interference;
        for (const auto& kv : blocks) {
            auto livenow = kv.second.liveout;
            // walk basic block backwards
            for (auto i = kv.second.last, e = kv.second.first; i != e; --i) {
                i->def_apply([&] (std::string& x) {
                    for (const auto& y : livenow) {
                        if (x != y) {
                            interference.emplace(
                                std::max(x, y),
                                std::min(x, y)
                            );
                        }
                    }
                    livenow.erase(x);
                });
                i->use_apply([&] (std::string& x) {
                    livenow.insert(x);
                });
            }
        }

        // find a coloring
        std::set<std::string> names;
        for (auto& instruction : instructions) {
            instruction.use_apply([&] (const std::string& x) {
                names.insert(x);
            });
            instruction.def_apply([&] (const std::string& x) {
                names.insert(x);
            });
        }

        auto colors = color(names, interference);

        // spill
        int spill_counter = 0;
        did_spill = false;
        for (auto i = instructions.begin(), e = instructions.end(); i != e;) {
            auto& instruction = *i;
            ++spill_counter;
            std::pair<std::string, std::string> load_spill;
            std::pair<std::string, std::string> store_spill;
            instruction.use_apply([&] (std::string& x) {
                if (colors.count(x)) {
                    auto c = colors.at(x);
                    if (c == color_unknown) {
                    } else if (c == color_spill) {
                        load_spill = std::make_pair("#SPILLED_" + x, x + "_" + std::to_string(spill_counter));
                        x = load_spill.second;
                        did_spill = true;
                    }
                }
            });
            instruction.def_apply([&] (std::string& x) {
                if (colors.count(x)) {
                    auto c = colors.at(x);
                    if (c == color_unknown) {
                    } else if (c == color_spill) {
                        store_spill = std::make_pair("#SPILLED_" + x, x + "_" + std::to_string(spill_counter));
                        x = store_spill.second;
                        did_spill = true;
                    }
                }
            });
            if (!load_spill.first.empty()) {
                instructions.emplace(i, instruction_type::LOAD, std::vector<std::string>{load_spill.second, load_spill.first} );
            }
            // meh
            if (!store_spill.first.empty()) {
                if (instruction.type == instruction_type::MOV && instruction.arguments[0] == store_spill.second) {
                    instruction.type = instruction_type::STORE;
                    instruction.arguments[0] = store_spill.first;
                    ++i;
                } else {
                    ++i;
                    instructions.emplace(i, instruction_type::STORE, std::vector<std::string>{store_spill.first, store_spill.second} );
                }
            } else {
                ++i;
            }
        }
        if (!did_spill) {
            for (auto i = instructions.begin(), e = instructions.end(); i != e;) {
                auto& instruction = *i;
                instruction.use_apply([&] (std::string& x) {
                    if (colors.count(x)) {
                        auto c = colors.at(x);
                        assert (c != color_unknown);
                        x = "%R" + std::to_string(c);
                    }
                });
                instruction.def_apply([&] (std::string& x) {
                    if (colors.count(x)) {
                        auto c = colors.at(x);
                        assert (c != color_unknown);
                        x = "%R" + std::to_string(c);
                    }
                });
                if (instruction.type == instruction_type::MOV && instruction.arguments[0] == instruction.arguments[1]) {
                    i = instructions.erase(i);
                } else {
                    ++i;
                }
            }
        }
    } while(did_spill);
}

}} // namespace hcc::ssa
