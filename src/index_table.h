// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#ifndef INDEX_TABLE_H
#define INDEX_TABLE_H

#include <cassert>
#include <algorithm>
#include <vector>

template <typename Derived, typename Key, typename Value>
struct index_table {
    Value put(const Key& key)
    {
        const auto p = std::find_if(pairs.begin(), pairs.end(),
                                    [&](const Pair& p) { return key == p.first; });
        if (p == pairs.end()) {
            pairs.emplace_back(key, Derived::construct(counter++));
            return pairs.back().second;
        } else {
            return p->second;
        }
    }

    const Key& get(const Value& value) const
    {
        const auto p = std::find_if(pairs.begin(), pairs.end(),
                                    [&](const Pair& p) { return value == p.second; });
        assert(p != pairs.end());
        return p->first;
    }

private:
    using Pair = std::pair<Key, Value>;
    std::vector<Pair> pairs;
    int counter{0};
};

#endif // INDEX_TABLE_H
