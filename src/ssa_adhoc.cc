// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <map>

namespace hcc { namespace ssa {


void subroutine::prettify_names(unsigned& var_counter)
{
    std::map<std::string, unsigned> var_replacement;
    auto var_collector = [&] (std::string& s) {
        auto it = var_replacement.find(s);
        if (it == var_replacement.end()) {
            var_replacement.emplace(s, var_counter++);
        }
    };
    auto var_replacer = [&] (std::string& s) {
        auto it = var_replacement.find(s);
        if (it != var_replacement.end()) {
            auto olds = s;
            s = "%" + std::to_string(it->second);
        }
    };

    for_each_bb([&] (basic_block& bb) {
    for (auto& instruction : bb.instructions) {
        instruction.use_apply(var_collector);
        instruction.def_apply(var_collector);
    }
    });
    for_each_bb([&] (basic_block& bb) {
    for (auto& instruction : bb.instructions) {
        instruction.use_apply(var_replacer);
        instruction.def_apply(var_replacer);
    }
    });
}


}} // namespace hcc::ssa
