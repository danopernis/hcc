// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <map>

namespace hcc { namespace ssa {


void subroutine::prettify_names(unsigned& var_counter, unsigned& label_counter)
{
    std::map<std::string, unsigned> var_replacement;
    auto var_collector = [&] (std::string& s) {
        auto it = var_replacement.find(s);
        if (it == var_replacement.end()) {
            var_replacement.emplace(s, var_counter++);
        }
    };
    auto var_replacer = [&] (std::string& s) {
        auto it = var_replacement.find(s);
        if (it != var_replacement.end()) {
            auto olds = s;
            s = "%" + std::to_string(it->second);
        }
    };

    std::map<std::string, unsigned> label_replacement;
    auto label_collector = [&] (std::string& s) {
        auto it = label_replacement.find(s);
        if (it == label_replacement.end()) {
            label_replacement.emplace(s, label_counter++);
        }
    };
    auto label_replacer = [&] (std::string& s) {
        auto it = label_replacement.find(s);
        if (it != label_replacement.end()) {
            auto olds = s;
            s = "L" + std::to_string(it->second);
        }
    };

    for (auto& instruction : instructions) {
        instruction.use_apply(var_collector);
        instruction.def_apply(var_collector);
        instruction.label_apply(label_collector);
    }
    for (auto& instruction : instructions) {
        instruction.use_apply(var_replacer);
        instruction.def_apply(var_replacer);
        instruction.label_apply(label_replacer);
    }
}


// clean cfg program with materialized PHI nodes
void subroutine::clean_cfg()
{
    struct block_info {
        bool discovered = false;
        instruction_list::iterator begin;
        instruction_list::iterator back;
        bool empty = true;
        std::set<std::string> predecessors;
    };

    // keep going while there are changes to control flow graph
    bool changed = true; // give algorithm a chance
    while (changed) {
        changed = false;

        // update blocks & successors info
        // FIXME don't recompute all the time
        std::multimap<std::string, std::string> successors;
        std::map<std::string, block_info> blocks;
        std::map<std::string, block_info>::iterator last_block;
        for (auto i = instructions.begin(), e = instructions.end(); i != e; ++i) {
            switch (i->type) {
            case instruction_type::LABEL:
                assert(i->arguments.size() == 1);
                last_block = blocks.emplace(i->arguments[0], block_info()).first;
                last_block->second.begin = i;
                break;
            case instruction_type::JUMP:
                assert(i->arguments.size() == 1);
                successors.emplace(last_block->first, i->arguments[0]);
                last_block->second.back = i;
                break;
            case instruction_type::BRANCH:
                assert(i->arguments.size() == 3);
                successors.emplace(last_block->first, i->arguments[1]);
                successors.emplace(last_block->first, i->arguments[2]);
                last_block->second.back = i;
                break;
            case instruction_type::RETURN:
                assert(i->arguments.size() == 1);
                last_block->second.back = i;
                break;
            default:
                last_block->second.empty = false;
                break;
            }
        }

        // compute postorder using depth-first graph traversal
        std::vector<std::string> postorder;
        std::function<void(std::string)> DFS = [&] (std::string v) {
            blocks.at(v).discovered = true;
            auto range = successors.equal_range(v);
            for (auto i = range.first; i != range.second; ++i) {
                const auto& w = i->second;
                blocks.at(w).predecessors.emplace(v);
                if (!blocks.at(w).discovered) {
                    DFS(w);
                }
            }
            postorder.push_back(v);
        };
        DFS(instructions.front().arguments[0]);

        for (auto& block : blocks) {
            if (block.second.discovered)
                continue;

            instructions.erase(block.second.begin, ++block.second.back);
        }

        for (const std::string& block_name : postorder) {
            auto& block = blocks.at(block_name);
            auto& back = block.back;

            // if block ends in a branch and both targets are identical
            if (back->type == instruction_type::BRANCH && back->arguments[1] == back->arguments[2]) {

                back->type = instruction_type::JUMP;
                back->arguments = { back->arguments[1] }; // choose either first or second
                changed = true;
            }

            if (back->type == instruction_type::JUMP) {
                auto& dest = blocks.at(back->arguments[0]);

                // if block contains nothing but a jump
                if (block.empty) {
                    for (const auto& pred : block.predecessors) {
                        // redirect predecessor to dest
                        auto& predecessor_back = *blocks.at(pred).back;

                        auto rewrite = [&] (std::string& x) {
                            if (x == block_name) {
                                x = back->arguments[0];
                                changed = true;
                            }
                        };

                        if (predecessor_back.type == instruction_type::JUMP) {
                            rewrite(predecessor_back.arguments[0]);
                        } else if (predecessor_back.type == instruction_type::BRANCH) {
                            rewrite(predecessor_back.arguments[1]);
                            rewrite(predecessor_back.arguments[2]);
                        } else {
                            assert(false && "predecessor is not a predecessor");
                        }

                    }
                } else if (dest.predecessors.size() == 1) {
                    dest.begin = instructions.erase(dest.begin); // erase label in destination block
                    instructions.splice(block.back, instructions, dest.begin, ++instruction_list::iterator(dest.back)); // include branch in dest
                    block.back = instructions.erase(block.back); // erase jump in current block

                    changed = true;
                } else if (dest.empty && dest.back->type == instruction_type::BRANCH) {
                    // TODO rewrite block's jump with dest's branch
                }
            }
            if (changed)
                break; // shame
        };
    }
    // done
}

}} // namespace hcc::ssa
