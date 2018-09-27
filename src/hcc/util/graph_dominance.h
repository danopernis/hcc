// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#pragma once

#include "hcc/util/graph.h"
#include <vector>
#include <set>

namespace hcc {
namespace util {

struct graph_dominance {
    std::vector<std::set<int>> dfs; // dominance frontier set
    graph tree;
    int root;

    graph_dominance(const graph& g, const int entry_node);
};

} // namespace util {
} // namespace hcc {
