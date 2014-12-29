// Copyright (c) 2013-2014 Dano Pernis
// See LICENSE for details

#include "ssa_subroutine_builder.h"

namespace hcc { namespace ssa {

subroutine_builder::subroutine_builder(subroutine_ir& s)
        : s(s)
{
    s.exit_node_ = add_bb("EXIT", false);
    add_instruction(s.exit_node_, instruction(instruction_type::RETURN, {"0"}));
}

int subroutine_builder::add_bb(const std::string& name, bool is_entry)
{
    auto it = name_to_index.find(name);
    if (it == name_to_index.end()) {
        int index = s.g.add_node();
        name_to_index.emplace(name, index);
        auto& node = s.basic_blocks[index];
        node.name = name;
        node.index = index;
        if (is_entry) {
            s.entry_node_ = index;
        }
        return index;
    } else {
        return it->second;
    }
}

void subroutine_builder::add_instruction(int bb, const instruction& instr)
{ s.basic_blocks.at(bb).instructions.push_back(instr); }

void subroutine_builder::add_jump(int bb, const std::string& target)
{
    s.g.add_edge(bb, add_bb(target, false));
    add_instruction(bb, instruction(
        instruction_type::JUMP, {target}));
}

void subroutine_builder::add_branch(int bb, const std::string& variable,
    const std::string& positive, const std::string& negative)
{
    s.g.add_edge(bb, add_bb(positive, false));
    s.g.add_edge(bb, add_bb(negative, false));
    add_instruction(bb, instruction(
        instruction_type::BRANCH, {variable, positive, negative}));
}

void subroutine_builder::add_return(int bb, const std::string& variable)
{
    s.g.add_edge(bb, s.exit_node_);
    add_instruction(bb, instruction(
        instruction_type::RETURN, {variable}));
}

}} // namespace hcc::ssa
