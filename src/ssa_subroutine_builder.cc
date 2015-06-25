// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "ssa_subroutine_builder.h"

namespace hcc { namespace ssa {

subroutine_builder::subroutine_builder(subroutine_ir& s)
        : s(s)
{
    s.exit_node_ = add_bb("EXIT");
    add_instruction(s.exit_node_, instruction(instruction_type::RETURN, {argument(constant(0))}));
}

label subroutine_builder::add_bb(const std::string& name, bool is_entry)
{
    auto it = name_to_index.find(name);
    if (it == name_to_index.end()) {
        label l;
        l.index = s.g.add_node();
        name_to_index.emplace(name, l);
        auto& node = s.basic_blocks[l];
        node.name = l;
        if (is_entry) {
            s.entry_node_ = l;
        }
        return l;
    } else {
        return it->second;
    }
}

void subroutine_builder::add_instruction(const label& bb, const instruction& instr)
{ s.basic_blocks.at(bb).instructions.push_back(instr); }

void subroutine_builder::add_jump(const label& bb, const label& target)
{
    s.g.add_edge(bb.index, target.index);
    add_instruction(bb, instruction(
        instruction_type::JUMP, {argument(target)}));
}

void subroutine_builder::add_branch(
    const label& bb,
    const instruction_type& type,
    const argument& variable1,
    const argument& variable2,
    const label& positive,
    const label& negative)
{
    s.g.add_edge(bb.index, positive.index);
    s.g.add_edge(bb.index, negative.index);
    add_instruction(bb, instruction(type,
        {variable1, variable2, argument(positive), argument(negative)}));
}

void subroutine_builder::add_return(const label& bb, const argument& variable)
{
    s.g.add_edge(bb.index, s.exit_node_.index);
    add_instruction(bb, instruction(
        instruction_type::RETURN, {variable}));
}

}} // namespace hcc::ssa
