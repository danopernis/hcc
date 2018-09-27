// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#include "graph.h"

void assert_order(const std::vector<int>& elements, const int first, const int second)
{
    bool seen_first = false;
    bool seen_second = false;
    for (const int element : elements) {
        if (element == first) {
            assert(!seen_first); // must be unique
            assert(!seen_second); // must keep order
            seen_first = true;
        } else if (element == second) {
            assert(!seen_second); // must be unique
            assert(seen_first); // must keep order
            seen_second = true;
        }
    }
}

//     ->-- n3
//    /
// n1 -->-- n2
void test_tree()
{
    graph g;
    int n1 = g.add_node();
    int n2 = g.add_node();
    int n3 = g.add_node();
    g.add_edge(n1, n2);
    g.add_edge(n1, n3);
    depth_first_search dfs(g.successors(), n1);

    assert(dfs.visited()[n1]);
    assert(dfs.visited()[n2]);
    assert(dfs.visited()[n3]);

    assert(dfs.preorder().size() == 3);
    assert_order(dfs.preorder(), n1, n2);
    assert_order(dfs.preorder(), n1, n3);

    assert(dfs.postorder().size() == 3);
    assert_order(dfs.postorder(), n2, n1);
    assert_order(dfs.postorder(), n3, n1);
}

//     ->-- n4
//    /
// n1 -->-- n2 -->-- n3
//             \    /
//              -<-
void test_cyclic()
{
    graph g;
    int n1 = g.add_node();
    int n2 = g.add_node();
    int n3 = g.add_node();
    int n4 = g.add_node();
    g.add_edge(n1, n2);
    g.add_edge(n2, n3);
    g.add_edge(n3, n2);
    g.add_edge(n1, n4);
    depth_first_search dfs(g.successors(), n1);

    assert(dfs.visited()[n1]);
    assert(dfs.visited()[n2]);
    assert(dfs.visited()[n3]);
    assert(dfs.visited()[n4]);

    assert(dfs.preorder().size() == 4);
    assert_order(dfs.preorder(), n1, n2);
    assert_order(dfs.preorder(), n1, n3);
    assert_order(dfs.preorder(), n1, n4);
    assert_order(dfs.preorder(), n2, n3);

    assert(dfs.postorder().size() == 4);
    assert_order(dfs.postorder(), n3, n2);
    assert_order(dfs.postorder(), n3, n1);
    assert_order(dfs.postorder(), n4, n1);
}

// n1 -->-- n2 --<-- n3
void test_orphan()
{
    graph g;
    int n1 = g.add_node();
    int n2 = g.add_node();
    int n3 = g.add_node();
    g.add_edge(n1, n2);
    g.add_edge(n3, n2);
    depth_first_search dfs(g.successors(), n1);

    assert(dfs.visited()[n1]);
    assert(dfs.visited()[n2]);
    assert(!dfs.visited()[n3]);

    assert(dfs.preorder().size() == 2);
    assert_order(dfs.preorder(), n1, n2);

    assert(dfs.postorder().size() == 2);
    assert_order(dfs.postorder(), n2, n1);
}

int main()
{
    test_tree();
    test_cyclic();
    test_orphan();
}
