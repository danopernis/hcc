// Copyright (c) 2013-2014 Dano Pernis
// See LICENSE for details

#ifndef SSA_SUBROUTINE_BUILDER_H
#define SSA_SUBROUTINE_BUILDER_H

#include "ssa.h"
#include <map>
#include <string>

namespace hcc { namespace ssa {

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
    int add_bb(const std::string& name, bool is_entry);

    /**
     * Add instruction to given basic block.
     */
    void add_instruction(int bb, const instruction& instr);
    void add_jump(int bb, const std::string& target);
    void add_branch(
        int bb,
        const instruction_type& type,
        const std::string& variable1,
        const std::string& variable2,
        const std::string& positive,
        const std::string& negative);
    void add_return(int bb, const std::string& variable);

private:
    std::map<std::string, int> name_to_index;
    subroutine_ir& s;
};

}} // namespace hcc::ssa

#endif // SSA_SUBROUTINE_BUILDER_H
