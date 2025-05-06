#include "GraphCSR.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <iostream>

GraphCSR GraphCSR::load_from_edgelist(const std::string& path,
    bool one_based,
    bool undirected)
{
    std::ifstream fin(path);
    if (!fin) throw std::runtime_error("Cannot open " + path);

    std::vector<std::pair<Vertex, Vertex>> edges;
    Vertex max_id = 0;
    std::string line;
    bool first_line = true;

    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#') continue;

        // Accept both CSV “u,v” and space “u v”
        std::replace(line.begin(), line.end(), ',', ' ');
        std::stringstream ss(line);
        Vertex u, v;
        if (!(ss >> u >> v)) {          // probably the header row
            if (first_line) { first_line = false; continue; }
            throw std::runtime_error("Bad line: " + line);
        }
        first_line = false;

        if (one_based) { --u; --v; }
        edges.emplace_back(u, v);
        max_id = std::max(max_id, std::max(u, v));
    }

    const Vertex n = max_id + 1;

    // ----- build CSR -----
    std::vector<Vertex> degree(n, 0);
    for (auto [a, b] : edges) {
        ++degree[a];
        if (undirected && a != b) ++degree[b];
    }

    GraphCSR g(n);
    g._offset.resize(n + 1);
    g._offset[0] = 0;
    for (Vertex i = 0; i < n; ++i)
        g._offset[i + 1] = g._offset[i] + degree[i];

    g._adj.resize(g._offset.back());
    g._w.assign(g._adj.size(), 1.0f);          // unweighted = 1

    std::vector<Vertex> cursor = g._offset;    // write pointers
    for (auto [a, b] : edges) {
        g._adj[cursor[a]++] = b;
        if (undirected && a != b)
            g._adj[cursor[b]++] = a;
    }
    return g;
}

void GraphCSR::print_stats() const
{
    std::cout << "Graph: n=" << _n
        << ", m=" << _adj.size() / 2
        << " (undirected edges)\n";
}

GraphCSR GraphCSR::build_from_edges(
    Vertex n,
    const std::vector<std::tuple<Vertex, Vertex, Weight>>& edges,
    bool undirected)
{
    std::vector<Vertex> degree(n, 0);
    for (auto [a, b, w] : edges) {
        ++degree[a];
        if (undirected && a != b) ++degree[b];
    }

    GraphCSR g(n);
    g._offset.resize(n + 1);
    g._offset[0] = 0;
    for (Vertex i = 0; i < n; ++i)
        g._offset[i + 1] = g._offset[i] + degree[i];

    g._adj.resize(g._offset.back());
    g._w.resize(g._adj.size());

    std::vector<Vertex> cursor = g._offset;
    for (auto [a, b, w] : edges) {
        g._adj[cursor[a]] = b;
        g._w[cursor[a]++] = w;
        if (undirected && a != b) {
            g._adj[cursor[b]] = a;
            g._w[cursor[b]++] = w;
        }
    }
    return g;
}
