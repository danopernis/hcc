// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include <map>
#include <string>
#include "ssa.h"

namespace hcc { namespace ssa {

void subroutine::copy_propagation()
{
    struct replacer_algorithm {
        void insert(const std::string& from, const std::string& to)
        {
            // replace[to] = from;
            // find if there's a chain
            auto it = replace.find(from);
            if (it != replace.end()) {
                replace[to] = it->second;
            } else {
                replace[to] = from;
            }
        }

        void operator()(std::string& s) const
        {
            auto it = replace.find(s);
            if (it != replace.end())
                s = it->second;
        }

        std::map<std::string, std::string> replace;
    } replacer;

    // pass 1: find MOVs and make replacement list
    for (auto& instruction : instructions) {
        if (instruction.type == instruction_type::MOV) {
            replacer.insert(instruction.arguments[1], instruction.arguments[0]);
        }
    }

    // pass 2: apply the replacement
    for (auto& instruction : instructions) {
//        if (instruction.type == instruction_type::PHI) {
            // TODO relax, dont propagate constant but do propagate variables
//        } else {
            instruction.use_apply(replacer);
//        }
    }
}

}} // namespace hcc::ssa
