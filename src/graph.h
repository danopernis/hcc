// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#pragma once
#include <cassert>
#include <functional>
#include <set>
#include <vector>

struct graph {
    /**
     * Construct an empty graph.
     */
    graph()
        : node_count_{0}
    {
    }

    /**
     * Add a node to graph.
     *
     * @return node index
     */
    int add_node()
    {
        successors_.emplace_back();
        predecessors_.emplace_back();
        return node_count_++;
    }

    /**
     * Add an edge to graph.
     *
     * @precondition from and to are valid node indices
     */
    void add_edge(const int from, const int to)
    {
        assert(0 <= from && from < node_count_);
        assert(0 <= to && to < node_count_);

        successors_[from].insert(to);
        predecessors_[to].insert(from);
    }

    /**
     * Remove an edge from graph.
     *
     * @precondition from and to are valid node indices
     */
    void remove_edge(const int from, const int to)
    {
        assert(0 <= from && from < node_count_);
        assert(0 <= to && to < node_count_);

        successors_[from].erase(to);
        predecessors_[to].erase(from);
    }

    /**
     * Get a graph with reversed edge directions.
     */
    graph reverse() const
    {
        graph result;
        result.node_count_ = node_count_;
        result.successors_ = predecessors_;
        result.predecessors_ = successors_;
        return result;
    }

    /** Accesors */
    int node_count() const { return node_count_; }
    const std::vector<std::set<int>>& successors() const { return successors_; }
    const std::vector<std::set<int>>& predecessors() const { return predecessors_; }

private:
    int node_count_;
    std::vector<std::set<int>> successors_;
    std::vector<std::set<int>> predecessors_;
};

struct depth_first_search {
    /**
     * Perform a depth-first search on the graph, computing preorder, postorder
     * and visited.
     *
     * @complexity at most O(node_count)
     * @precondition graph is not empty and root is valid node index
     */
    depth_first_search(const std::vector<std::set<int>>& successors, const int root)
    {
        const int node_count = successors.size();
        assert(node_count > 0);

        // reset
        preorder_ = std::vector<int>();
        postorder_ = std::vector<int>();
        visited_ = std::vector<bool>(node_count, false);

        // define recurrsion
        std::function<void(int)> search = [&](int node) {
            assert(0 <= node && node < node_count);

            visited_[node] = true;
            preorder_.push_back(node);
            for (const int successor : successors[node]) {
                assert(0 <= successor && successor < node_count);

                if (!visited_[successor]) {
                    search(successor);
                }
            }
            postorder_.push_back(node);
        };

        // start with the root node
        search(root);
    }

    /** Accesors */
    const std::vector<int>& preorder() const { return preorder_; }
    const std::vector<int>& postorder() const { return postorder_; }
    const std::vector<bool>& visited() const { return visited_; }

private:
    std::vector<int> preorder_;
    std::vector<int> postorder_;
    std::vector<bool> visited_;
};
