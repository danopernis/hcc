// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#pragma once

#include "hcc/ssa/ssa.h"

#include <map>
#include <string>

namespace hcc {
namespace ssa {

struct subroutine_builder {
    /**
     * Prepare subroutine builder to act on subroutine.
     *
     * precondition: subroutine is empty
     */
    subroutine_builder(subroutine_ir& s);

    /**
     * Add basic block to subroutine.
     *
     * Inserts new basic block if it is not already present and returns handle.
     */
    label add_bb(const std::string& name, bool is_entry = false);

    reg add_reg(const std::string& name);

    local add_local(const std::string& name);

    /**
     * Add instruction to given basic block.
     */
    void add_instruction(const label& bb, const instruction& instr);
    void add_jump(const label& bb, const label& target);
    void add_branch(const label& bb, const instruction_type& type, const argument& variable1,
                    const argument& variable2, const label& positive, const label& negative);
    void add_return(const label& bb, const argument& variable);

private:
    std::map<std::string, label> lookup_label;
    std::map<std::string, reg> lookup_reg;
    std::map<std::string, local> lookup_local;
    subroutine_ir& s;
};

} // namespace ssa {
} // namespace hcc {
