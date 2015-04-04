// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include <map>
#include <string>
#include "ssa.h"

namespace hcc { namespace ssa {

void subroutine::copy_propagation()
{
    struct replacer_algorithm {
        void insert(const argument& from, const argument& to)
        {
            // replace[to] = from;
            // find if there's a chain
            auto it = replace.find(from);
            if (it != replace.end()) {
                replace.emplace(to, it->second).first->second = it->second;
            } else {
                replace.emplace(to, from).first->second = from;
            }
        }

        void operator()(argument& a) const
        {
            auto it = replace.find(a);
            if (it != replace.end())
                a = it->second;
        }

        std::map<argument, argument> replace;
    } replacer;

    // pass 1: find MOVs and make replacement list
    for_each_bb([&] (basic_block& bb) {
    for (auto& instruction : bb.instructions) {
        if (instruction.type == instruction_type::MOV) {
            const auto& src = instruction.arguments[1];
            const auto& dst = instruction.arguments[0];
            replacer.insert(src, dst);
        }
    }
    });

    // pass 2: apply the replacement
    for_each_bb([&] (basic_block& bb) {
    for (auto& instruction : bb.instructions) {
        instruction.use_apply(replacer);
    }
    });
}

}} // namespace hcc::ssa
