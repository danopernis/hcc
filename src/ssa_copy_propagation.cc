// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include <map>
#include <string>
#include "ssa.h"

namespace hcc { namespace ssa {

void subroutine::copy_propagation()
{
    std::map<argument, argument> replace;
    auto replacer = [&] (argument& a) {
        auto it = replace.find(a);
        if (it != replace.end()) {
            a = it->second;
        }
    };

    // pass 1: find MOVs and make replacement list
    for_each_bb([&] (basic_block& bb) {
    for (auto& instruction : bb.instructions) {
        if (instruction.type == instruction_type::MOV) {
            const auto& src = instruction.arguments[1];
            const auto& dst = instruction.arguments[0];
            // replace[dst] = src;
            // find if there's a chain
            auto it = replace.find(src);
            if (it != replace.end()) {
                replace.emplace(dst, it->second).first->second = it->second;
            } else {
                replace.emplace(dst, src).first->second = src;
            }
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
