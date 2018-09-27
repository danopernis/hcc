// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#include "hcc/ssa/interference_graph.h"
#include <cassert>

using interference_graph_type = hcc::ssa::interference_graph<int, int>;

void no_node_no_color()
{
    std::vector<int> nodes;
    std::vector<int> colors;
    interference_graph_type ig{nodes, colors};
    auto result = ig.find_coloring();

    assert(!result);
}

void no_node_two_colors()
{
    std::vector<int> nodes;
    std::vector<int> colors = {11, 12};
    interference_graph_type ig{nodes, colors};
    auto result = ig.find_coloring();

    assert(!result);
}

void one_node_no_color()
{
    std::vector<int> nodes{1};
    std::vector<int> colors;
    interference_graph_type ig{nodes, colors};
    auto result = ig.find_coloring();

    assert(result);
    assert(*result == 1);
}

void one_node_no_color_self_reference()
{
    std::vector<int> nodes{1};
    std::vector<int> colors;
    interference_graph_type ig{nodes, colors};
    ig.add_edge(1, 1);
    auto result = ig.find_coloring();

    assert(result);
    assert(*result == 1);
}

void one_node_one_color()
{
    std::vector<int> nodes{1};
    std::vector<int> colors{11};
    interference_graph_type ig{nodes, colors};
    auto result = ig.find_coloring();

    assert(!result);
    assert(ig.get_color(1) == 11);
}

void one_node_one_color_self_reference()
{
    std::vector<int> nodes{1};
    std::vector<int> colors{11};
    interference_graph_type ig{nodes, colors};
    ig.add_edge(1, 1);
    auto result = ig.find_coloring();

    assert(!result);
    assert(ig.get_color(1) == 11);
}

void one_node_two_colors()
{
    std::vector<int> nodes{1};
    std::vector<int> colors{11, 12};
    interference_graph_type ig{nodes, colors};
    auto result = ig.find_coloring();

    assert(!result);
    assert(ig.get_color(1) == 11 || ig.get_color(1) == 12);
}

void one_node_two_colors_self_reference()
{
    std::vector<int> nodes{1};
    std::vector<int> colors{11, 12};
    interference_graph_type ig{nodes, colors};
    ig.add_edge(1, 1);
    auto result = ig.find_coloring();

    assert(!result);
    assert(ig.get_color(1) == 11 || ig.get_color(1) == 12);
}

void two_nodes_no_color()
{
    std::vector<int> nodes{1, 2};
    std::vector<int> colors;
    interference_graph_type ig{nodes, colors};
    auto result = ig.find_coloring();

    assert(result);
    assert(*result == 1 || *result == 2);
}

void two_nodes_one_color()
{
    std::vector<int> nodes{1, 2};
    std::vector<int> colors{11};
    interference_graph_type ig{nodes, colors};
    auto result = ig.find_coloring();

    assert(!result);
    assert(ig.get_color(1) == 11);
    assert(ig.get_color(2) == 11);
}

void two_nodes_one_color_connected()
{
    std::vector<int> nodes{1, 2};
    std::vector<int> colors{11};
    interference_graph_type ig{nodes, colors};
    ig.add_edge(1, 2);
    auto result = ig.find_coloring();

    assert(result);
    assert(*result == 1 || *result == 2);
}

void two_nodes_two_colors()
{
    std::vector<int> nodes{1, 2};
    std::vector<int> colors{11, 12};
    interference_graph_type ig{nodes, colors};
    auto result = ig.find_coloring();

    assert(!result);
    assert(ig.get_color(1) == 11 || ig.get_color(1) == 12);
    assert(ig.get_color(2) == 11 || ig.get_color(2) == 12);
}

void two_nodes_two_colors_connected()
{
    std::vector<int> nodes{1, 2};
    std::vector<int> colors{11, 12};
    interference_graph_type ig{nodes, colors};
    ig.add_edge(1, 2);
    auto result = ig.find_coloring();

    assert(!result);
    assert((ig.get_color(1) == 11 && ig.get_color(2) == 12)
           || (ig.get_color(1) == 12 && ig.get_color(2) == 11));
}

//   1---2
//  / \ /
// 3---4
void spill_break_most_edges()
{
    std::vector<int> nodes{1, 2, 3, 4};
    std::vector<int> colors{11, 12};
    interference_graph_type ig{nodes, colors};
    ig.add_edge(1, 2);
    ig.add_edge(1, 4);
    ig.add_edge(2, 4);
    ig.add_edge(3, 1);
    ig.add_edge(3, 4);
    auto result = ig.find_coloring();

    assert(result);
    assert(*result == 1 || *result == 4);
}

int main()
{
    no_node_no_color();
    no_node_two_colors();

    one_node_no_color();
    one_node_no_color_self_reference();
    one_node_one_color();
    one_node_one_color_self_reference();
    one_node_two_colors();
    one_node_two_colors_self_reference();

    two_nodes_no_color();
    two_nodes_one_color();
    two_nodes_one_color_connected();
    two_nodes_two_colors();
    two_nodes_two_colors_connected();

    spill_break_most_edges();

    return 0;
}
