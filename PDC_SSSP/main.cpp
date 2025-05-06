#include "SsspUpdater.hpp"
#include "EdgeList.hpp"
#include "Change.hpp"
#include <iostream>
#include <random>
#include <limits>

int main() {
    try {
        /* ---------- 1. load GrQc ---------- */
        auto gfile = GraphCSR::load_from_edgelist(
            "data/grqc.edgelist", false, true);

        const std::size_t N = gfile.num_vertices();
        std::cout << "Loaded GrQc: n=" << N
            << ", m=" << gfile.adj().size() / 2 << " edges\n";

        /* ---------- 2. build EdgeList & degree[] ---------- */
        EdgeList edges;
        std::vector<int> degree(N, 0);

        for (Vertex u = 0; u < N; ++u)
            for (Vertex ei = gfile.offsets()[u]; ei < gfile.offsets()[u + 1]; ++ei) {
                Vertex v = gfile.adj()[ei];
                edges.add(u, v);
                ++degree[u];
            }

        /* ---------- 3. choose the highest‑degree source ---------- */
        Vertex src = std::distance(degree.begin(),
            std::max_element(degree.begin(), degree.end()));
        std::cout << "Using source vertex " << src
            << " (degree " << degree[src] << ")\n";

        SerialUpdater up(edges);
        up.initialise(src);

        /* ---------- 4. pick a target with distance ≥ 5 ---------- */
        const float INF = std::numeric_limits<float>::infinity();
        Vertex tgt = src;
        float  best = 0.0f;

        for (Vertex v = 0; v < N; ++v) {
            float d = up.dist()[v];
            if (d < INF && d > best && d >= 5) {   // require at least 5 hops
                best = d;
                tgt = v;
            }
        }
        if (tgt == src) {
            std::cerr << "No vertex at dist ≥ 5 — increase threshold or check graph\n";
            return 1;
        }
        std::cout << "[baseline] vertex " << tgt << ", dist = " << best << '\n';

        /* ---------- 5. build a batch of 1 000 random edits ---------- */
        std::mt19937 rng(123);
        std::uniform_int_distribution<int> vdist(0, N - 1);
        std::bernoulli_distribution insert_prob(0.5);

        std::vector<Change> batch;
        while (batch.size() < 1000) {
            Vertex a = vdist(rng), b = vdist(rng);
            if (a == b) continue;
            batch.push_back({ insert_prob(rng) ? Op::Insert : Op::Delete, a, b });
        }

        /* ---------- 6. apply & report ---------- */
        up.apply_changes(batch, src);
        std::cout << "[after 1000 edits] dist[" << tgt << "] = "
            << up.dist()[tgt] << '\n';

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << '\n';
        return 1;
    }
}
