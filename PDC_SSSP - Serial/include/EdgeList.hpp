#pragma once
#include <vector>
#include <tuple>
#include <unordered_set>

struct EdgeHash {
    std::size_t operator()(const std::pair<Vertex, Vertex>& p) const noexcept {
        return (static_cast<std::size_t>(p.first) << 32) ^ p.second;
    }
};

// Undirected edge container; keeps duplicates out and is cheap to mutate
class EdgeList {
public:
    void add(Vertex u, Vertex v, Weight w = 1.0f) {
        auto key = canon(u, v);
        if (_set.insert(key).second)
            _vec.emplace_back(key.first, key.second, w);
    }
    void remove(Vertex u, Vertex v) {
        _set.erase(canon(u, v));          // lazy erase from vec
    }

    const std::vector<std::tuple<Vertex, Vertex, Weight>>& vec() const { return _vec; }
    Vertex max_vertex_id() const {
        Vertex m = 0;
        for (auto [u, v, w] : _vec) m = std::max(m, std::max(u, v));
        return m;
    }

private:
    static std::pair<Vertex, Vertex> canon(Vertex a, Vertex b) {
        return (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
    }
    std::unordered_set<std::pair<Vertex, Vertex>, EdgeHash> _set;
    std::vector<std::tuple<Vertex, Vertex, Weight>>         _vec;
};
