// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#pragma once
#include <boost/optional.hpp>
#include <stack>
#include <vector>

template <typename Name, typename Color>
struct interference_graph {

    template <typename NameContainer, typename ColorContainer>
    interference_graph(const NameContainer& names, const ColorContainer& colors_)
        : nodes{begin(names), end(names)}
        , colors{begin(colors_), end(colors_)}
        , edges(nodes.size() * nodes.size(), false)
    {
        std::sort(begin(nodes), end(nodes), node::compare_name);
        std::sort(begin(colors), end(colors));
    }

    void add_edge(const Name& x, const Name& y)
    {
        if (!(x == y)) {
            auto i = find_name(x) - begin(nodes);
            auto j = find_name(y) - begin(nodes);
            edges[i * nodes.size() + j] = true;
            edges[j * nodes.size() + i] = true;
        }
    }

    // return none if coloring was found; else return best candidate for spilling
    boost::optional<Name> find_coloring() { return reconstruct(deconstruct()); }

    // precondition: find_coloring()
    Color get_color(const Name& name) const { return *find_name(name)->color; }

private:
    struct node {
        node(const Name& name)
            : name{name}
        {
        }

        static bool compare_name(const node& a, const node& b) { return a.name < b.name; }

        Name name;
        boost::optional<Color> color;
        bool marked{false};
    };
    using node_list = typename std::vector<node>;
    using node_iterator = typename node_list::iterator;
    using const_node_iterator = typename node_list::const_iterator;

    void mark_removed(node_iterator node)
    {
        node->marked = true;
        ++marked_count;
    }

    void unmark_removed(node_iterator node)
    {
        node->marked = false;
        --marked_count;
    }

    bool empty() const { return marked_count == nodes.size(); }

    const_node_iterator find_name(const Name& name) const
    {
        auto result = std::lower_bound(begin(nodes), end(nodes), node{name}, node::compare_name);
        assert(result != end(nodes));
        assert(result->name == name);
        return result;
    }

    template <typename F>
    void for_each_neighbour(const_node_iterator node, F&& f) const
    {
        const auto row = node - begin(nodes);
        auto rowit = begin(edges) + row * nodes.size();
        for (auto i = begin(nodes), e = end(nodes); i != e; ++i) {
            const auto is_edge = *rowit++;
            if (is_edge && !i->marked) {
                f(i);
            }
        }
    }

    int get_count(const_node_iterator node) const
    {
        int result = 0;
        for_each_neighbour(node, [&result](const_node_iterator) { ++result; });
        return result;
    }

    std::vector<Color> collect_neighbour_colors(const_node_iterator node) const
    {
        std::vector<Color> result;
        for_each_neighbour(node, [&result](const_node_iterator n) { result.push_back(*n->color); });
        std::sort(begin(result), end(result));
        result.erase(std::unique(begin(result), end(result)), end(result));
        return result;
    }

    node_iterator select_node()
    {
        auto node = nodes.begin();
        while (node->marked) {
            ++node;
        }
        auto max = node;
        int max_count = -1;
        for (; node != end(nodes); ++node) {
            if (node->marked) {
                continue;
            }
            const auto count = get_count(node);
            if (count < colors.size()) {
                return node;
            }
            if (count > max_count) {
                max_count = count;
                max = node;
            }
        }
        return max;
    }

    std::stack<node_iterator> deconstruct()
    {
        std::stack<node_iterator> stack;
        while (!empty()) {
            auto node = select_node();
            stack.push(node);
            mark_removed(node);
        }
        return stack;
    }

    boost::optional<Name> reconstruct(std::stack<node_iterator> stack)
    {
        while (!stack.empty()) {
            auto node = stack.top();
            stack.pop();
            unmark_removed(node);

            const auto neighbour_colors = collect_neighbour_colors(node);
            if (neighbour_colors.size() < colors.size()) {
                node->color = *std::mismatch(begin(neighbour_colors), end(neighbour_colors),
                                             begin(colors)).second;
            } else {
                return node->name;
            }
        }
        return boost::none;
    }

    node_list nodes;
    std::vector<Color> colors;
    std::vector<bool> edges;
    unsigned marked_count{0};
};
