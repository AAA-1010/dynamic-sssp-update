#include "SsspUpdater.hpp"
#ifdef _OPENMP
#include <omp.h>
#endif
#include <queue>
#include <utility>
#include <algorithm>

/* ---------- internal helpers ---------- */
namespace {
    using Pair = std::pair<float, Vertex>;   // (dist,u)
}

/* rebuild CSR from edge list */
static GraphCSR rebuild(const EdgeList& edges) {
    Vertex n = edges.max_vertex_id() + 1;
    return GraphCSR::build_from_edges(n, edges.vec(), true);
}

/* ---------- ctor ---------- */
SerialUpdater::SerialUpdater(const EdgeList& edges)
    : _edges(edges),
    _g(rebuild(_edges))
{
    Vertex n = _g.num_vertices();
    _dist.assign(n, INF);
    _parent.assign(n, static_cast<Vertex>(-1));
    _affected.assign(n, 0);
    _valid.assign(n, 1);
    _child_head.assign(n, static_cast<Vertex>(-1));
    _next_sib.assign(n, static_cast<Vertex>(-1));
}

/* ---------- full Dijkstra ---------- */
void SerialUpdater::initialise(Vertex source)
{
    _g = rebuild(_edges);
    Vertex n = _g.num_vertices();
    _dist.assign(n, INF);
    _parent.assign(n, static_cast<Vertex>(-1));
    _affected.assign(n, 0);
    _valid.assign(n, 1);

    _dist[source] = 0.0f;
    std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>> pq;
    pq.emplace(0.0f, source);

    const auto& off = _g.offsets();
    const auto& adj = _g.adj();
    const auto& w = _g.w();

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d != _dist[u]) continue;
        for (Vertex ei = off[u]; ei < off[u + 1]; ++ei) {
            Vertex v = adj[ei];
            float nd = d + w[ei];
            if (nd < _dist[v]) {
                _dist[v] = nd;
                _parent[v] = u;
                pq.emplace(nd, v);
            }
        }
    }
    build_tree_from_parents();
}

/* ---------- helpers ---------- */
void SerialUpdater::build_tree_from_parents()
{
    std::fill(_child_head.begin(), _child_head.end(), static_cast<Vertex>(-1));
    std::fill(_next_sib.begin(), _next_sib.end(), static_cast<Vertex>(-1));

    for (Vertex v = 0; v < _parent.size(); ++v) {
        Vertex p = _parent[v];
        if (p == static_cast<Vertex>(-1)) continue;
        _next_sib[v] = _child_head[p];
        _child_head[p] = v;
    }
}

void SerialUpdater::mark_subtree_invalid(Vertex root)
{
    std::vector<Vertex> stk = { root };
    while (!stk.empty()) {
        Vertex u = stk.back(); stk.pop_back();
        _valid[u] = 0;
        _affected[u] = 1;
        _dist[u] = INF;
        for (Vertex c = _child_head[u]; c != static_cast<Vertex>(-1); c = _next_sib[c])
            stk.push_back(c);
    }
}

void SerialUpdater::relax(Vertex u, Vertex v, Weight w, bool& any_change)
{
    if (!_valid[u]) return;
    float nd = _dist[u] + w;
    if (nd < _dist[v]) {
        _dist[v] = nd;
        _parent[v] = u;
        _affected[v] = 1;
        any_change = true;
    }
}

/* ---------- incremental update (OpenMP) ---------- */
void SerialUpdater::apply_changes(const std::vector<Change>& batch,
    Vertex source)
{
    /* STEP A – first‑order effects */
#pragma omp parallel for schedule(static)
    for (int i = 0; i < static_cast<int>(batch.size()); ++i) {
        const auto& ch = batch[i];
        bool dummy = false;

        if (ch.op == Op::Insert) {
#pragma omp critical(edge_mod)
            _edges.add(ch.u, ch.v, ch.w);

            relax(ch.u, ch.v, ch.w, dummy);
            relax(ch.v, ch.u, ch.w, dummy);
        }
        else { // Delete
#pragma omp critical(edge_mod)
            _edges.remove(ch.u, ch.v);

            if (_parent[ch.u] == ch.v)      mark_subtree_invalid(ch.u);
            else if (_parent[ch.v] == ch.u) mark_subtree_invalid(ch.v);
        }
    }

    /* rebuild CSR once */
    _g = rebuild(_edges);

    /* STEP B – propagation loop */
    const auto& off = _g.offsets();
    const auto& adj = _g.adj();
    const auto& w = _g.w();

    bool again = true;
    while (again) {
        again = false;

#pragma omp parallel for schedule(dynamic,256) reduction(|:again)
        for (int i = 0; i < static_cast<int>(_g.num_vertices()); ++i) {
            Vertex u = static_cast<Vertex>(i);

            if (!_affected[u]) continue;
            _affected[u] = 0;

            for (Vertex ei = off[u]; ei < off[u + 1]; ++ei)
                relax(u, adj[ei], w[ei], again);

            _valid[u] = 1;
        }
    }
    build_tree_from_parents();
}
