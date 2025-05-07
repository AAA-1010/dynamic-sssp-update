#include "SsspUpdater.hpp"
#include "EdgeList.hpp"
#include "Change.hpp"

#include <iostream>
#include <fstream>       // for CSV I/O
#include <random>
#include <limits>
#include <vector>
#include <algorithm>
#include <chrono>        // for high_resolution_clock

#ifdef _OPENMP
#include <omp.h>
#endif

/* ---------- things we can tweak in code ---------- */
constexpr std::size_t BATCH_SIZE = 15000;   
constexpr unsigned     RNG_SEED = 123;
const char* GRAPH_FILE = "data/roadNet-CA.edgelist";
//const char* GRAPH_FILE = "data/grqc.edgelist";


/* ---------- helper: extract src → tgt path ---------- */
std::vector<Vertex> extract_path(const SerialUpdater& up, Vertex src, Vertex tgt) {
    std::vector<Vertex> p;
    if (up.dist()[tgt] >= std::numeric_limits<float>::infinity())
        return p;
    for (Vertex v = tgt; v != src; v = up.parent()[v])
        p.push_back(v);
    p.push_back(src);
    std::reverse(p.begin(), p.end());
    return p;
}

int main() {
    // ——— CSV header check ———
    std::ifstream in("results.csv");
    bool needHeader = !in.good() || in.peek() == std::ifstream::traits_type::eof();
    in.close();
    std::ofstream csv("results.csv", std::ios::app);
    if (needHeader) {
        csv << "dataset,batch_size,time_ms,version\n";
    }

    // ——— Startup message ———
    int threads = 1;
#ifdef _OPENMP
    threads = omp_get_max_threads();
#endif
    std::cout << "=== Dynamic SSSP (OpenMP, " << threads << " threads) ===\n";

    // 1. load graph
    auto g = GraphCSR::load_from_edgelist(GRAPH_FILE, /*one_based=*/false, /*undirected=*/true);
    std::cout << "Graph: n=" << g.num_vertices()
        << ", m=" << g.adj().size() / 2 << " edges\n";

    // 2. build EdgeList & degree[]
    EdgeList edges;
    std::vector<int> degree(g.num_vertices(), 0);
    for (Vertex u = 0; u < g.num_vertices(); ++u)
        for (Vertex ei = g.offsets()[u]; ei < g.offsets()[u + 1]; ++ei) {
            edges.add(u, g.adj()[ei]);
            ++degree[u];
        }

    // 3. choose source (highest-degree)
    Vertex src = std::distance(degree.begin(), std::max_element(degree.begin(), degree.end()));
    std::cout << "Source vertex = " << src
        << "  (degree " << degree[src] << ")\n";

    SerialUpdater up(edges);
    up.initialise(src);

    // 4. choose target at distance ≥ 5
    const float INF = std::numeric_limits<float>::infinity();
    Vertex tgt = src; float best = 0.0f;
    for (Vertex v = 0; v < g.num_vertices(); ++v) {
        float d = up.dist()[v];
        if (d < INF && d >= 5 && d > best) {
            best = d; tgt = v;
        }
    }
    if (tgt == src) {
        std::cerr << "No vertex at distance ≥ 5 found; adjust threshold.\n";
        return 1;
    }

    // 5. print baseline
    auto basePath = extract_path(up, src, tgt);
    std::cout << "\nBaseline: dist = " << best << "\n  path: ";
    for (size_t i = 0; i < basePath.size(); ++i) {
        std::cout << basePath[i]
            << (i + 1 < basePath.size() ? " -> " : "\n");
    }

    // 6. build random batch
    std::mt19937                       rng(RNG_SEED);
    std::uniform_int_distribution<int> vdist(0, g.num_vertices() - 1);
    std::bernoulli_distribution        opdist(0.5);
    std::vector<Change> batch;
    batch.reserve(BATCH_SIZE);
    while (batch.size() < BATCH_SIZE) {
        Vertex a = vdist(rng), b = vdist(rng);
        if (a == b) continue;
        batch.push_back({ opdist(rng) ? Op::Insert : Op::Delete, a, b });
    }

    // 7. apply & time
    auto t0 = std::chrono::high_resolution_clock::now();
    up.apply_changes(batch, src);
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // write to CSV with version tag
    csv << "roadNet-CA," << BATCH_SIZE << "," << elapsed_ms << ",openmp\n";

    // 8. print new path & distance
    std::cout << "\nAfter " << BATCH_SIZE
        << " edits (" << elapsed_ms << " ms):\n";
    float newDist = up.dist()[tgt];
    if (newDist >= INF) {
        std::cout << "target unreachable\n";
    }
    else {
        auto newPath = extract_path(up, src, tgt);
        std::cout << "dist = " << newDist << "\n  path: ";
        for (size_t i = 0; i < newPath.size(); ++i) {
            std::cout << newPath[i]
                << (i + 1 < newPath.size() ? " -> " : "\n");
        }
    }

    return 0;
}
