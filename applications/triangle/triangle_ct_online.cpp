#include "../../src/CliqueTwinCore.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

using namespace twbench;
using MsClock = std::chrono::steady_clock;

static double elapsed_ms(MsClock::time_point a, MsClock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

static std::vector<Vertex> degree_rank_pos(const CSRGraph& g) {
    std::vector<Vertex> order(g.n), pos(g.n);
    std::iota(order.begin(), order.end(), Vertex{0});
    std::stable_sort(order.begin(), order.end(), [&](Vertex a, Vertex b) {
        return g.degree[a] > g.degree[b];
    });
    for (Vertex i = 0; i < g.n; ++i) pos[order[i]] = i;
    return pos;
}

struct DirectedResidual {
    std::vector<std::uint64_t> offsets;
    std::vector<Vertex> edges;
    std::vector<Word> neighbor_and;
};

static DirectedResidual build_directed_residual(const CliqueTwinIndex& ct,
                                                 const std::vector<Vertex>& pos) {
    DirectedResidual d;
    d.offsets.assign(static_cast<std::size_t>(ct.residual.n) + 1, 0);
    d.neighbor_and.assign(ct.residual.n, ~Word{0});

    for (Vertex u = 0; u < ct.residual.n; ++u) {
        for (std::uint64_t p = ct.residual.offsets[u]; p < ct.residual.offsets[u + 1]; ++p) {
            const Vertex v = ct.residual.neighbors[p];
            if (pos[v] < pos[u]) {
                ++d.offsets[u + 1];
                d.neighbor_and[u] &= (ct.header[v] & ct.gt_mask);
            }
        }
    }
    std::partial_sum(d.offsets.begin(), d.offsets.end(), d.offsets.begin());
    d.edges.resize(d.offsets.back());
    auto cursor = d.offsets;
    for (Vertex u = 0; u < ct.residual.n; ++u) {
        for (std::uint64_t p = ct.residual.offsets[u]; p < ct.residual.offsets[u + 1]; ++p) {
            const Vertex v = ct.residual.neighbors[p];
            if (pos[v] < pos[u]) d.edges[cursor[u]++] = v;
        }
    }
    return d;
}

static std::uint64_t count_residual_triangles(const DirectedResidual& d,
                                               std::uint64_t& aggregate_skips) {
    std::vector<std::uint8_t> mark(d.neighbor_and.size(), 0);
    std::uint64_t ans = 0;
    aggregate_skips = 0;
    for (Vertex u = 0; u < d.neighbor_and.size(); ++u) {
        if (d.neighbor_and[u] != 0) {
            ++aggregate_skips;
            continue;
        }
        const std::uint64_t begin = d.offsets[u], end = d.offsets[u + 1];
        if (end - begin < 2) continue;
        for (std::uint64_t p = begin; p < end; ++p) mark[d.edges[p]] = 1;
        for (std::uint64_t p = begin; p < end; ++p) {
            const Vertex v = d.edges[p];
            for (std::uint64_t q = d.offsets[v]; q < d.offsets[v + 1]; ++q) {
                ans += mark[d.edges[q]];
            }
        }
        for (std::uint64_t p = begin; p < end; ++p) mark[d.edges[p]] = 0;
    }
    return ans;
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: ct_triangle_online <graph> [clique-threshold]\n";
        return 1;
    }
    try {
        const int threshold = argc == 3 ? std::stoi(argv[2]) : 5;
        ParseStats ps;
        const CSRGraph g = read_graph(argv[1], ps);

        const auto b0 = MsClock::now();
        const CliqueTwinIndex ct = build_clique_twin(g, threshold);
        const auto b1 = MsClock::now();

        const auto prep0 = MsClock::now();
        const auto pos = degree_rank_pos(g);
        const DirectedResidual residual = build_directed_residual(ct, pos);
        const auto prep1 = MsClock::now();

        const auto a0 = MsClock::now();
        std::vector<std::uint32_t> clique_size(static_cast<std::size_t>(ct.stats.clique_count) + 1, 0);
        for (Vertex v = 0; v < g.n; ++v) {
            const std::uint32_t p = ct.pid(v);
            if (p != 0) ++clique_size[p];
        }
        std::uint64_t part_a = 0;
        for (std::size_t p = 1; p < clique_size.size(); ++p) {
            const std::uint64_t s = clique_size[p];
            if (s >= 3) part_a += s * (s - 1) * (s - 2) / 6;
        }
        const auto a1 = MsClock::now();

        const auto bb0 = MsClock::now();
        std::vector<std::uint32_t> count(static_cast<std::size_t>(ct.stats.clique_count) + 1, 0);
        std::vector<Vertex> stamp(static_cast<std::size_t>(ct.stats.clique_count) + 1,
                                  std::numeric_limits<Vertex>::max());
        std::uint64_t part_b = 0;
        for (Vertex v = 0; v < g.n; ++v) {
            const std::uint32_t pv = ct.pid(v);
            for (std::uint64_t e = g.offsets[v]; e < g.offsets[v + 1]; ++e) {
                const std::uint32_t p = ct.pid(g.neighbors[e]);
                if (p == 0 || p == pv) continue;
                if (stamp[p] != v) { stamp[p] = v; count[p] = 0; }
                part_b += count[p];
                ++count[p];
            }
        }
        const auto bb1 = MsClock::now();

        const auto c0 = MsClock::now();
        std::uint64_t aggregate_skips = 0;
        const std::uint64_t part_c = count_residual_triangles(residual, aggregate_skips);
        const auto c1 = MsClock::now();

        const std::uint64_t triangles = part_a + part_b + part_c;
        std::cout << std::fixed << std::setprecision(3)
                  << "triangles=" << triangles << "\n"
                  << "partA_clique=" << part_a << "\n"
                  << "partB_two_clique_vertices=" << part_b << "\n"
                  << "partC_residual=" << part_c << "\n"
                  << "cliques=" << ct.stats.clique_count << " pid_bits=" << ct.stats.k0
                  << " gt_prime_bits=" << ct.stats.gt_bits << "\n"
                  << "removed_edges=" << ct.stats.clique_edges_removed
                  << " residual_edges=" << ct.stats.residual_edges << "\n"
                  << "aggregate_skips=" << aggregate_skips << "\n"
                  << "index_build_ms=" << elapsed_ms(b0,b1) << "\n"
                  << "triangle_prep_ms=" << elapsed_ms(prep0,prep1) << "\n"
                  << "phaseA_ms=" << elapsed_ms(a0,a1) << "\n"
                  << "phaseB_ms=" << elapsed_ms(bb0,bb1) << "\n"
                  << "phaseC_ms=" << elapsed_ms(c0,c1) << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
}
