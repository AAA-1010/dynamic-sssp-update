#pragma once
#include <vector>
#include <string>
#include <cstdint>

using Vertex = std::uint32_t;
using Weight = float;          // set to int if you prefer

class GraphCSR {
public:
    explicit GraphCSR(Vertex n = 0) : _n(n) {}

    // Load an edge‑list file (CSV or space‑separated, header optional)
    static GraphCSR load_from_edgelist(const std::string& path,
        bool one_based = true,
        bool undirected = true);

    static GraphCSR build_from_edges(
        Vertex n,
        const std::vector<std::tuple<Vertex, Vertex, Weight>>& edges,
        bool undirected = true);



    // Quick stats helper (optional)
    void print_stats() const;

    // Accessors
    Vertex num_vertices() const { return _n; }
    const std::vector<Vertex>& offsets() const { return _offset; }
    const std::vector<Vertex>& adj()     const { return _adj; }
    const std::vector<Weight>& w()       const { return _w; }

private:
    Vertex                _n = 0;
    std::vector<Vertex>   _offset;   // size n+1
    std::vector<Vertex>   _adj;      // size 2m (undirected)
    std::vector<Weight>   _w;        // weights parallel to _adj
};
