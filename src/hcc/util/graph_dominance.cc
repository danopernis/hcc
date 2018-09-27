// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/util/graph_dominance.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <vector>

namespace hcc {
namespace util {

namespace {

/**
 * Find immediate dominator for each vertex in a control flow graph, using
 * an algorithm proposed by Cooper, Harvey & Kennedy.
 *
 * @precondition preds and their indices are in reversed post-order, numbered from zero.
 */
std::pair<std::vector<unsigned>, // idom
          std::multimap<unsigned, unsigned> // dfs
          > inline chk_internal(const std::vector<std::vector<unsigned>>& preds)
{
    assert(preds.size() > 0);

    // initialization
    std::vector<unsigned> idom(preds.size());
    std::vector<bool> idom_defined(preds.size(), false);
    idom[0] = 0;
    idom_defined[0] = true;

    // iterate until we converged
    bool changed = true;
    while (changed) {
        changed = false;

        // for all nodes except the first
        for (unsigned b = 1; b < preds.size(); ++b) {
            auto firstPred = preds[b].begin();
            auto lastPred = preds[b].end();
            assert(firstPred != lastPred);

            // pick the first predecessor of b without loss of generality
            unsigned newidom = *firstPred++;

            // for each other predecessor p of b
            for_each(firstPred, lastPred, [&](unsigned p) {
                if (idom_defined[p]) {
                    // newidom = intersect(p, newidom)
                    unsigned finger1 = p;
                    unsigned finger2 = newidom;

                    while (finger1 != finger2) {
                        while (finger1 > finger2)
                            finger1 = idom[finger1];
                        while (finger2 > finger1)
                            finger2 = idom[finger2];
                    }
                    newidom = finger1;
                }
            });

            // if we found better candidate for immediate dominator, save it
            if (!idom_defined[b] || idom[b] != newidom) {
                idom_defined[b] = true;
                idom[b] = newidom;
                changed = true;
            }
        }
    }

    // while we're at it, compute dominance frontier as well
    std::multimap<unsigned, unsigned> dfs;
    for (unsigned b = 0; b < preds.size(); ++b) {
        if (preds[b].size() < 2)
            continue;

        for (auto p : preds[b]) {
            unsigned runner = p;
            while (runner != idom[b]) {
                dfs.insert(std::make_pair(runner, b));
                runner = idom[runner];
            }
        }
    }

    return std::make_pair(idom, dfs);
}

} // namespace {

graph_dominance::graph_dominance(const graph& g, const int entry_node)
    : dfs(g.node_count())
    , root(entry_node)
{
    const depth_first_search DFS(g.successors(), entry_node);

    // alright, we calculated post-order visitation
    // now we need a mapping from block name to reverse post-order
    std::map<unsigned, unsigned> translate;
    std::map<unsigned, unsigned> translate_back;
    unsigned j;
    j = 0;
    for (auto i = DFS.postorder().rbegin(), e = DFS.postorder().rend(); i != e; ++i) {
        translate.insert(std::make_pair(*i, j));
        translate_back.insert(std::make_pair(j, *i));
        ++j;
    }
    std::vector<std::vector<unsigned>> parents_int;
    for (auto i = DFS.postorder().rbegin(), e = DFS.postorder().rend(); i != e; ++i) {
        std::vector<unsigned> ok_parents;
        for (int p : g.predecessors()[*i]) {
            // of course, parents are indices to old, scrambled array
            // let's translate them
            ok_parents.push_back(translate.at(p));
        }
        parents_int.push_back(ok_parents);
    }

    // actual computation
    auto result = chk_internal(parents_int);

    // translate back
    j = 0;
    for (int i = 0; i < g.node_count(); ++i) {
        int n = tree.add_node();
        assert(i == n);
    }
    for (auto id : result.first) {
        int from = translate_back.at(id);
        int to = translate_back.at(j++);
        if (from != to)
            tree.add_edge(from, to);
    }
    for (auto kv : result.second) {
        dfs[translate_back.at(kv.first)].insert(translate_back.at(kv.second));
    }
}

} // namespace util {
} // namespace hcc {
