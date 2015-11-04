// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <cassert>

namespace hcc {
namespace ssa {

void instruction::use_apply(std::function<void(argument&)> g)
{
    auto f = [&](argument& a) {
        if (a.is_reg()) {
            g(a);
        }
    };

    switch (type) {
    case instruction_type::CALL: {
        int counter = 0;
        for (auto& argument : arguments) {
            if (counter >= 2) {
                f(argument);
            }
            ++counter;
        }
    } break;
    case instruction_type::PHI: {
        int counter = 0;
        for (auto& argument : arguments) {
            if (counter != 0 && counter % 2 == 0) {
                f(argument);
            }
            ++counter;
        }
    } break;
    case instruction_type::RETURN:
        f(arguments[0]);
        break;
    case instruction_type::JUMP:
    case instruction_type::ARGUMENT:
        break;
    case instruction_type::STORE:
    case instruction_type::JLT:
    case instruction_type::JEQ:
        f(arguments[0]);
        f(arguments[1]);
        break;
    case instruction_type::ADD:
    case instruction_type::SUB:
    case instruction_type::AND:
    case instruction_type::OR:
        f(arguments[1]);
        f(arguments[2]);
        break;
    case instruction_type::LOAD:
    case instruction_type::MOV:
    case instruction_type::NEG:
    case instruction_type::NOT:
        f(arguments[1]);
        break;
    default:
        assert(false);
    }
}

void subroutine_ir::recompute_dominance()
{
    // hack: graph_dominance can't handle blocks that are not target of jump
    depth_first_search dfs(g.successors(), entry_node_.index);
    for (auto& node : basic_blocks) {
        const auto& from = node.first;
        if (!dfs.visited()[from.index]) {
            for (const auto& to : g.successors()[from.index]) {
                g.remove_edge(from.index, to);
                basic_blocks.at(from).instructions.clear();
            }
        }
    }

    dominance.reset(new graph_dominance(g, entry_node_.index));
    reverse_dominance.reset(new graph_dominance(g.reverse(), exit_node_.index));
}

void subroutine_ir::recompute_liveness()
{
    // init
    for (auto& kv : basic_blocks) {
        auto& block = kv.second;
        block.uevar.clear();
        block.varkill.clear();
        block.liveout.clear();
        for (auto& instruction : block.instructions) {
            instruction.use_apply([&block](const argument& a) {
                if (!a.is_reg()) {
                    return;
                }
                const auto& x = a.get_reg();
                if (!block.varkill.count(x)) {
                    block.uevar.insert(x);
                }
            });
            instruction.def_apply([&block](const argument& a) {
                if (!a.is_reg()) {
                    return;
                }
                const auto& x = a.get_reg();
                block.varkill.insert(x);
            });
        }
    }

    // iterate
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& kv : basic_blocks) {
            auto& block = kv.second;
            auto old_liveout = block.liveout;

            std::set<reg> new_liveout;
            for_each_cfg_successor(block.name, [&](basic_block& bb) {
                const auto& uevar = bb.uevar;
                const auto& varkill = bb.varkill;
                const auto& liveout = bb.liveout;
                std::copy(uevar.begin(), uevar.end(),
                          std::inserter(new_liveout, new_liveout.begin()));
                for (const auto& x : liveout) {
                    if (varkill.count(x) == 0)
                        new_liveout.insert(x);
                }
            });
            block.liveout = new_liveout;

            if (block.liveout != old_liveout) {
                changed = true;
            }
        }
    }
}

std::set<reg> subroutine_ir::collect_variable_names()
{
    std::set<reg> result;
    auto inserter = [&](argument& a) {
        if (a.is_reg()) {
            result.insert(a.get_reg());
        }
    };
    for_each_bb([&](basic_block& bb) {
        for (auto& instruction : bb.instructions) {
            instruction.use_apply(inserter);
            instruction.def_apply(inserter);
        }
    });
    return result;
}
}
} // end namespace hcc::ssa
