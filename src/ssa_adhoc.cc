// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <map>

namespace hcc { namespace ssa {


void subroutine::prettify_names(unsigned& var_counter, unsigned& label_counter)
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

    std::map<std::string, unsigned> label_replacement;
    auto label_collector = [&] (std::string& s) {
        auto it = label_replacement.find(s);
        if (it == label_replacement.end()) {
            label_replacement.emplace(s, label_counter++);
        }
    };
    auto label_replacer = [&] (std::string& s) {
        auto it = label_replacement.find(s);
        if (it != label_replacement.end()) {
            auto olds = s;
            s = "L" + std::to_string(it->second);
        }
    };

    for (auto& instruction : instructions) {
        instruction.use_apply(var_collector);
        instruction.def_apply(var_collector);
        instruction.label_apply(label_collector);
    }
    for (auto& instruction : instructions) {
        instruction.use_apply(var_replacer);
        instruction.def_apply(var_replacer);
        instruction.label_apply(label_replacer);
    }
}


}} // namespace hcc::ssa
