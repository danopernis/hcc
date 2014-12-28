// Copyright (c) 2013-2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <cassert>

namespace hcc { namespace ssa {

void instruction::use_apply(std::function<void(std::string&)> g)
{
    auto f = [&] (std::string& s) {
        if (!s.empty() && s[0] == '%')
            g(s);
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
    case instruction_type::BRANCH:
        f(arguments[0]);
        break;
    case instruction_type::JUMP:
    case instruction_type::ARGUMENT:
        break;
    case instruction_type::STORE:
        f(arguments[0]);
        f(arguments[1]);
        break;
    case instruction_type::ADD:
    case instruction_type::SUB:
    case instruction_type::AND:
    case instruction_type::OR:
    case instruction_type::LT:
    case instruction_type::GT:
    case instruction_type::EQ:
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

void instruction::def_apply(std::function<void(std::string&)> f)
{
    switch (type) {
    case instruction_type::BRANCH:
    case instruction_type::JUMP:
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
    case instruction_type::LT:
    case instruction_type::GT:
    case instruction_type::EQ:
    case instruction_type::PHI:
    case instruction_type::ARGUMENT:
        f(arguments[0]);
        break;
    default:
        assert(false);
    }
}

subroutine_ir::subroutine_ir()
{
    exit_node_ = add_bb("EXIT").index;
    basic_blocks[exit_node_].instructions.push_back(instruction(instruction_type::RETURN, {"0"}));

    entry_node_ = add_bb("ENTRY").index;
}

void subroutine_ir::recompute_dominance()
{
    // hack: graph_dominance can't handle blocks that are not target of jump
    depth_first_search dfs(g.successors(), entry_node_);
    for (auto& node : basic_blocks) {
        const auto& from = node.first;
        if (!dfs.visited()[from]) {
            for (const auto& to : g.successors()[from]) {
                g.remove_edge(from, to);
                basic_blocks[from].instructions.clear();
            }
        }
    }

    dominance.reset(new graph_dominance(g, entry_node_));
    reverse_dominance.reset(new graph_dominance(g.reverse(), exit_node_));
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
            instruction.use_apply([&block] (const std::string& x) {
                if (!block.varkill.count(x))
                    block.uevar.insert(x);
            });
            instruction.def_apply([&block] (const std::string& x) {
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

            std::set<std::string> new_liveout;
            for_each_cfg_successor(block.index, [&] (basic_block& bb) {
                const auto& uevar = bb.uevar;
                const auto& varkill = bb.varkill;
                const auto& liveout = bb.liveout;
                std::copy(uevar.begin(), uevar.end(), std::inserter(new_liveout, new_liveout.begin()));
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

std::set<std::string> subroutine_ir::collect_variable_names()
{
    std::set<std::string> result;
    auto inserter = [&] (std::string& s) { result.insert(s); };
    for_each_bb([&] (basic_block& bb) {
    for (auto& instruction : bb.instructions) {
        instruction.use_apply(inserter);
        instruction.def_apply(inserter);
    }
    });
    return result;
}

}} // end namespace hcc::ssa
