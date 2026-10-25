#pragma once

#include "GraphUtils.hpp"

namespace twbench {

struct CliqueTwinStats {
    int threshold = 5;
    std::uint32_t k0 = 0;
    std::uint32_t gt_bits = 64;
    std::uint64_t clique_count = 0;
    std::uint64_t clique_vertices = 0;
    std::uint64_t clique_edges_removed = 0;
    std::uint64_t residual_edges = 0;
    std::uint32_t max_clique_size = 0;
    double clique_extract_ms = 0.0;
    double residual_build_ms = 0.0;
    double gt_prime_build_ms = 0.0;
    double pack_ms = 0.0;
    double total_build_ms = 0.0;
};

// CliqueTwin uses a fixed 64-bit dual-certificate header:
// 1) greedily extract vertex-disjoint cliques (default threshold 5, supplemented at 4 and 3),
// 2) encode each clique by a nonzero PID in the high k0 bits,
// 3) delete intra-clique edges and build GraphTwin GT' on the residual graph,
// 4) use same-PID as a direct edge certificate, GT' overlap as a direct nonedge certificate,
//    and otherwise fall back to exact lookup in the residual graph.
class CliqueTwinIndex {
public:
    std::vector<Word> header;
    CSRGraph residual;
    Word pid_mask = 0;
    Word gt_mask = ~Word{0};
    CliqueTwinStats stats;

    std::uint64_t header_bytes() const noexcept { return header.size() * sizeof(Word); }
    std::uint64_t query_index_bytes() const noexcept { return header_bytes() + residual.csr_bytes(); }

    inline std::uint32_t pid(Vertex v) const noexcept {
        if (pid_mask == 0) return 0;
        const std::uint32_t shift = 64U - stats.k0;
        return static_cast<std::uint32_t>((header[v] & pid_mask) >> shift);
    }

    inline Decision decide(Vertex u, Vertex v) const noexcept {
        const Word hu = header[u];
        const Word hv = header[v];
        const Word pu = hu & pid_mask;
        if (pu != 0 && pu == (hv & pid_mask)) return Decision::Edge;
        if (((hu & hv) & gt_mask) != 0) return Decision::NonEdge;
        return Decision::Fallback;
    }

    inline bool edge_query(Vertex u, Vertex v) const noexcept {
        const Decision d = decide(u, v);
        if (d == Decision::Edge) return true;
        if (d == Decision::NonEdge) return false;
        return residual.has_edge(u, v);
    }
};

namespace detail {

struct CliqueExtractor {
    const CSRGraph& g;
    std::vector<std::uint32_t> current_degree;
    std::vector<std::uint8_t> used;
    std::vector<std::uint32_t> mark;
    std::uint32_t tag = 0;
    std::uint32_t max_degree = 0;
    std::vector<std::vector<Vertex>> cliques;

    explicit CliqueExtractor(const CSRGraph& graph) : g(graph) {
        current_degree = g.degree;
        used.assign(g.n, 0);
        mark.assign(g.n, 0);
        max_degree = g.max_degree;
    }

    std::uint64_t search(int threshold, std::vector<std::vector<Vertex>>& buckets,
                         std::uint64_t limit = std::numeric_limits<std::uint64_t>::max()) {
        std::uint64_t added = 0;
        for (std::int64_t d = static_cast<std::int64_t>(max_degree); d >= 0; --d) {
            auto& bucket = buckets[static_cast<std::size_t>(d)];
            while (!bucket.empty()) {
                const Vertex seed = bucket.back();
                bucket.pop_back();
                if (used[seed] || current_degree[seed] != static_cast<std::uint32_t>(d)) continue;

                std::vector<Vertex> clique;
                std::vector<Vertex> candidates;
                clique.push_back(seed);
                candidates.reserve(g.degree[seed]);
                for (std::uint64_t i = g.offsets[seed]; i < g.offsets[seed + 1]; ++i) {
                    const Vertex nb = g.neighbors[i];
                    if (!used[nb]) candidates.push_back(nb);
                }

                while (!candidates.empty()) {
                    Vertex best = candidates.front();
                    for (Vertex y : candidates) {
                        if (current_degree[y] > current_degree[best]) best = y;
                    }
                    clique.push_back(best);
                    ++tag;
                    if (tag == 0) {
                        std::fill(mark.begin(), mark.end(), 0);
                        tag = 1;
                    }
                    for (std::uint64_t i = g.offsets[best]; i < g.offsets[best + 1]; ++i) {
                        mark[g.neighbors[i]] = tag;
                    }
                    std::vector<Vertex> next;
                    next.reserve(candidates.size());
                    for (Vertex y : candidates) {
                        if (y != best && !used[y] && mark[y] == tag) next.push_back(y);
                    }
                    candidates.swap(next);
                }

                if (static_cast<int>(clique.size()) < threshold) continue;
                for (Vertex u : clique) used[u] = 1;
                for (Vertex u : clique) {
                    for (std::uint64_t i = g.offsets[u]; i < g.offsets[u + 1]; ++i) {
                        const Vertex nb = g.neighbors[i];
                        if (!used[nb]) {
                            --current_degree[nb];
                            buckets[current_degree[nb]].push_back(nb);
                        }
                    }
                }
                cliques.push_back(std::move(clique));
                if (++added >= limit) return added;
            }
        }
        return added;
    }

    void extract(int threshold) {
        std::vector<std::vector<Vertex>> buckets(static_cast<std::size_t>(max_degree) + 1);
        for (Vertex v = 0; v < g.n; ++v) buckets[current_degree[v]].push_back(v);
        search(threshold, buckets);

        const std::uint64_t c = cliques.size();
        const std::uint32_t initial_k0 = c == 0 ? 0U : static_cast<std::uint32_t>(std::ceil(std::log2(static_cast<double>(c + 1))));
        const std::uint64_t target = initial_k0 == 0 ? 0ULL : ((Word{1} << initial_k0) - 1ULL);
        std::uint64_t missing = target > c ? target - c : 0;
        for (int th = threshold - 1; th >= 3 && missing > 0; --th) {
            std::vector<std::vector<Vertex>> buckets(static_cast<std::size_t>(max_degree) + 1);
            for (Vertex v = 0; v < g.n; ++v) if (!used[v]) buckets[current_degree[v]].push_back(v);
            const std::uint64_t added = search(th, buckets, missing);
            missing -= std::min(missing, added);
        }
    }
};


inline void append_cliquetwin_gt_prime_labels(const CSRGraph& g, int bit_count, std::vector<Word>& labels) {
    std::vector<Vertex> order(g.n);
    std::iota(order.begin(), order.end(), Vertex{0});
    std::vector<std::uint32_t> selected_count(g.n, 0);
    std::vector<std::int8_t> state(g.n, -1);
    for (int bit = 0; bit < bit_count; ++bit) {
        if (bit == 0 || bit == 1) {
            std::vector<std::vector<Vertex>> buckets(static_cast<std::size_t>(g.max_degree) + 1);
            for (Vertex v : order) buckets[g.degree[v]].push_back(v);
            order.clear();
            if (bit == 0) {
                for (std::uint32_t d = 0; d <= g.max_degree; ++d)
                    order.insert(order.end(), buckets[d].begin(), buckets[d].end());
            } else {
                for (std::int64_t d = static_cast<std::int64_t>(g.max_degree); d >= 0; --d)
                    order.insert(order.end(), buckets[static_cast<std::size_t>(d)].begin(), buckets[static_cast<std::size_t>(d)].end());
            }
        } else {
            std::vector<std::vector<Vertex>> buckets(static_cast<std::size_t>(bit) + 1);
            for (Vertex v : order) buckets[selected_count[v]].push_back(v);
            order.clear();
            for (int c = 0; c <= bit; ++c) order.insert(order.end(), buckets[c].begin(), buckets[c].end());
        }
        run_greedy_mis(g, order, state);
        const Word mask = Word{1} << bit;
        for (Vertex v = 0; v < g.n; ++v) if (state[v] == 1) {
            labels[v] |= mask;
            ++selected_count[v];
        }
    }
}

inline std::uint32_t ceil_log2_plus_one(std::uint64_t c) {
    if (c == 0) return 0;
    std::uint32_t k = 0;
    std::uint64_t x = c;
    while (x) { ++k; x >>= 1; }
    return k;
}

} // namespace detail

inline CliqueTwinIndex build_clique_twin(const CSRGraph& g, int threshold = 5) {
    const auto total_start = Clock::now();
    CliqueTwinIndex out;
    out.stats.threshold = threshold;

    auto t0 = Clock::now();
    detail::CliqueExtractor extractor(g);
    extractor.extract(threshold);
    out.stats.clique_extract_ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    auto& cliques = extractor.cliques;
    std::uint32_t k0 = detail::ceil_log2_plus_one(cliques.size());
    if (k0 > 32) {
        std::sort(cliques.begin(), cliques.end(), [](const auto& a, const auto& b) { return a.size() > b.size(); });
        k0 = 32;
        const std::uint64_t max_cliques = (Word{1} << 32) - 1ULL;
        if (cliques.size() > max_cliques) cliques.resize(static_cast<std::size_t>(max_cliques));
    }
    out.stats.k0 = k0;
    out.stats.gt_bits = 64 - k0;
    out.stats.clique_count = cliques.size();

    std::vector<std::uint32_t> pid(g.n, 0);
    std::uint32_t id = 0;
    for (const auto& c : cliques) {
        ++id;
        out.stats.max_clique_size = std::max(out.stats.max_clique_size, static_cast<std::uint32_t>(c.size()));
        out.stats.clique_vertices += c.size();
        out.stats.clique_edges_removed += static_cast<std::uint64_t>(c.size()) * (c.size() - 1) / 2;
        for (Vertex v : c) pid[v] = id;
    }

    t0 = Clock::now();
    std::vector<Edge> residual_edges;
    residual_edges.reserve(g.edges.size() - std::min<std::uint64_t>(g.edges.size(), out.stats.clique_edges_removed));
    for (const auto& [u, v] : g.edges) {
        if (pid[u] != 0 && pid[u] == pid[v]) continue;
        residual_edges.emplace_back(u, v);
    }
    out.residual = make_csr(g.n, std::move(residual_edges));
    out.stats.residual_edges = out.residual.edges.size();
    out.stats.residual_build_ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    t0 = Clock::now();
    std::vector<Word> gt_prime(g.n, 0);
    if (out.stats.gt_bits > 0) detail::append_cliquetwin_gt_prime_labels(out.residual, static_cast<int>(out.stats.gt_bits), gt_prime);
    out.stats.gt_prime_build_ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    t0 = Clock::now();
    if (k0 == 0) {
        out.pid_mask = 0;
        out.gt_mask = ~Word{0};
    } else {
        out.gt_mask = (Word{1} << (64 - k0)) - 1ULL;
        out.pid_mask = ~out.gt_mask;
    }
    out.header.resize(g.n);
    for (Vertex v = 0; v < g.n; ++v) {
        const Word pid_part = k0 == 0 ? 0ULL : (Word(pid[v]) << (64 - k0));
        out.header[v] = pid_part | (gt_prime[v] & out.gt_mask);
    }
    out.stats.pack_ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    out.stats.total_build_ms = std::chrono::duration<double, std::milli>(Clock::now() - total_start).count();
    return out;
}

} // namespace twbench
