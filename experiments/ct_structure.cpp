#include "../src/CliqueTwinCore.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace twbench;

static std::uint32_t degeneracy(const CSRGraph& g) {
    if (g.n == 0) return 0;
    std::vector<std::uint32_t> degree = g.degree;
    std::vector<std::uint64_t> bin(static_cast<std::size_t>(g.max_degree) + 1, 0);
    for (Vertex v = 0; v < g.n; ++v) ++bin[degree[v]];
    std::uint64_t start = 0;
    for (std::uint32_t d = 0; d <= g.max_degree; ++d) {
        const auto n = bin[d]; bin[d] = start; start += n;
    }
    std::vector<Vertex> vert(g.n);
    std::vector<std::uint64_t> pos(g.n);
    for (Vertex v = 0; v < g.n; ++v) { pos[v] = bin[degree[v]]; vert[pos[v]] = v; ++bin[degree[v]]; }
    for (std::int64_t d = static_cast<std::int64_t>(g.max_degree); d >= 1; --d) bin[d] = bin[d-1];
    bin[0] = 0;
    std::uint32_t rho = 0;
    for (Vertex i = 0; i < g.n; ++i) {
        const Vertex v = vert[i];
        rho = std::max(rho, degree[v]);
        for (std::uint64_t e = g.offsets[v]; e < g.offsets[v+1]; ++e) {
            const Vertex u = g.neighbors[e];
            if (degree[u] > degree[v]) {
                const std::uint32_t du = degree[u];
                const auto pu = pos[u], pw = bin[du];
                const Vertex w = vert[pw];
                if (u != w) { std::swap(vert[pu], vert[pw]); pos[u] = pw; pos[w] = pu; }
                ++bin[du]; --degree[u];
            }
        }
    }
    return rho;
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: ct_structure <graph> [clique-threshold]\n";
        return 1;
    }
    try {
        const int threshold = argc == 3 ? std::stoi(argv[2]) : 5;
        ParseStats ps;
        const CSRGraph g = read_graph(argv[1], ps);
        const std::uint32_t rho_g = degeneracy(g);
        const CliqueTwinIndex ct = build_clique_twin(g, threshold);
        const std::uint32_t rho_r = degeneracy(ct.residual);

        std::cout << "n,m,rho_original,rho_residual,cliques,clique_vertices,removed_edges,residual_edges,pid_bits,gt_prime_bits,build_ms\n";
        std::cout << g.n << ',' << g.m() << ',' << rho_g << ',' << rho_r << ','
                  << ct.stats.clique_count << ',' << ct.stats.clique_vertices << ','
                  << ct.stats.clique_edges_removed << ',' << ct.stats.residual_edges << ','
                  << ct.stats.k0 << ',' << ct.stats.gt_bits << ',' << ct.stats.total_build_ms << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
}
