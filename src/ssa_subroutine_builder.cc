// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#include "ssa_subroutine_builder.h"

namespace hcc {
namespace ssa {

subroutine_builder::subroutine_builder(subroutine_ir& s)
    : s(s)
{
    add_instruction(s.exit_node_, instruction(instruction_type::RETURN, {argument(constant(0))}));
}

label subroutine_builder::add_bb(const std::string& name, bool is_entry)
{
    if (is_entry) {
        return s.entry_node_;
    }

    auto it = lookup_label.find(name);
    if (it == lookup_label.end()) {
        auto l = s.create_label();
        lookup_label.emplace(name, l);
        return l;
    } else {
        return it->second;
    }
}

reg subroutine_builder::add_reg(const std::string& name)
{
    auto it = lookup_reg.find(name);
    if (it == lookup_reg.end()) {
        auto r = s.create_reg();
        s.add_debug(r, "name", name);
        lookup_reg.emplace(name, r);
        return r;
    } else {
        return it->second;
    }
}

void subroutine_builder::add_instruction(const label& bb, const instruction& instr)
{
    s.basic_blocks.at(bb).instructions.push_back(instr);
}

void subroutine_builder::add_jump(const label& bb, const label& target)
{
    s.add_edge(bb, target);
    add_instruction(bb, instruction(instruction_type::JUMP, {argument(target)}));
}

void subroutine_builder::add_branch(const label& bb, const instruction_type& type,
                                    const argument& variable1, const argument& variable2,
                                    const label& positive, const label& negative)
{
    s.add_edge(bb, positive);
    s.add_edge(bb, negative);
    add_instruction(
        bb, instruction(type, {variable1, variable2, argument(positive), argument(negative)}));
}

void subroutine_builder::add_return(const label& bb, const argument& variable)
{
    s.add_edge(bb, s.exit_node_);
    add_instruction(bb, instruction(instruction_type::RETURN, {variable}));
}

} // namespace ssa {
} // namespace hcc {
