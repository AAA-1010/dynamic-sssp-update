#pragma once
#include "GraphCSR.hpp"
#include "EdgeList.hpp"
#include "Change.hpp"
#include <vector>
#include <limits>

class SerialUpdater {
public:
    explicit SerialUpdater(const EdgeList& edges);

    // baseline run (full Dijkstra) – still useful to “reset”
    void initialise(Vertex source = 0);

    // *** incremental batch update (Khanda et al.) ***
    void apply_changes(const std::vector<Change>& batch, Vertex source = 0);

    // getters
    const std::vector<float>& dist()   const { return _dist; }
    const std::vector<Vertex>& parent() const { return _parent; }
    const GraphCSR& graph()  const { return _g; }

private:
    EdgeList               _edges;
    GraphCSR               _g;
    std::vector<float>     _dist;
    std::vector<Vertex>    _parent;

    // incremental‑specific state
    std::vector<char>      _affected;
    std::vector<char>      _valid;
    std::vector<Vertex>    _child_head, _next_sib;

    static constexpr float INF = std::numeric_limits<float>::infinity();

    // helpers
    void build_tree_from_parents();
    void mark_subtree_invalid(Vertex root);
    void relax(Vertex u, Vertex v, Weight w, bool& any_change);
};
