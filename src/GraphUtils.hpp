#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace twbench {

using Vertex = std::uint32_t;
using Word = std::uint64_t;
using Edge = std::pair<Vertex, Vertex>;
using Clock = std::chrono::steady_clock;

enum class Decision : std::uint8_t { NonEdge = 0, Edge = 1, Fallback = 2 };

struct ParseStats {
    bool header_detected = false;
    std::string id_mode;
    std::uint64_t raw_lines = 0;
    std::uint64_t self_loops_removed = 0;
    std::uint64_t duplicates_removed = 0;
};

struct CSRGraph {
    Vertex n = 0;
    std::vector<std::uint64_t> offsets;
    std::vector<Vertex> neighbors;
    std::vector<Edge> edges;
    std::vector<std::uint32_t> degree;
    std::uint32_t max_degree = 0;

    std::uint64_t m() const noexcept { return edges.size(); }

    bool has_edge(Vertex u, Vertex v) const noexcept {
        const auto first = neighbors.begin() + static_cast<std::ptrdiff_t>(offsets[u]);
        const auto last = neighbors.begin() + static_cast<std::ptrdiff_t>(offsets[u + 1]);
        return std::binary_search(first, last, v);
    }

    std::uint64_t csr_bytes() const noexcept {
        return offsets.size() * sizeof(std::uint64_t) + neighbors.size() * sizeof(Vertex);
    }
};

inline std::uint64_t mix64(std::uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

struct SplitMix64Rng {
    using result_type = std::uint64_t;
    std::uint64_t state;
    explicit SplitMix64Rng(std::uint64_t seed) : state(seed) {}
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
    result_type operator()() { state += 0x9e3779b97f4a7c15ULL; return mix64(state); }
};

inline CSRGraph make_csr(Vertex n, std::vector<Edge> edges) {
    CSRGraph g;
    g.n = n;
    g.edges = std::move(edges);
    g.offsets.assign(static_cast<std::size_t>(n) + 1, 0);
    for (const auto& [u, v] : g.edges) {
        ++g.offsets[u + 1];
        ++g.offsets[v + 1];
    }
    std::partial_sum(g.offsets.begin(), g.offsets.end(), g.offsets.begin());
    g.neighbors.resize(g.offsets.back());
    auto cursor = g.offsets;
    for (const auto& [u, v] : g.edges) {
        g.neighbors[cursor[u]++] = v;
        g.neighbors[cursor[v]++] = u;
    }
    g.degree.resize(n);
    for (Vertex v = 0; v < n; ++v) {
        auto first = g.neighbors.begin() + static_cast<std::ptrdiff_t>(g.offsets[v]);
        auto last = g.neighbors.begin() + static_cast<std::ptrdiff_t>(g.offsets[v + 1]);
        std::sort(first, last);
        g.degree[v] = static_cast<std::uint32_t>(g.offsets[v + 1] - g.offsets[v]);
        g.max_degree = std::max(g.max_degree, g.degree[v]);
    }
    return g;
}

inline bool parse_pair_line(const std::string& line, std::uint64_t& a, std::uint64_t& b) {
    const char* p = line.c_str();
    while (*p && std::isspace(static_cast<unsigned char>(*p))) ++p;
    if (!*p || *p == '#' || *p == '%' || *p == 'c') return false;
    char* end = nullptr;
    a = std::strtoull(p, &end, 10);
    if (end == p) return false;
    p = end;
    while (*p && std::isspace(static_cast<unsigned char>(*p))) ++p;
    if (!*p) return false;
    b = std::strtoull(p, &end, 10);
    return end != p;
}

inline CSRGraph read_graph(const std::string& path, ParseStats& ps) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open graph: " + path);
    std::vector<std::pair<std::uint64_t, std::uint64_t>> raw;
    std::string line;
    while (std::getline(in, line)) {
        std::uint64_t a = 0, b = 0;
        if (parse_pair_line(line, a, b)) raw.emplace_back(a, b);
    }
    ps.raw_lines = raw.size();
    if (raw.empty()) throw std::runtime_error("no integer pairs in graph");

    bool header = false;
    std::uint64_t declared_n = 0;
    std::size_t begin = 0;
    if (raw.size() >= 2) {
        const auto [cn, cm] = raw.front();
        std::uint64_t max_id = 0;
        for (std::size_t i = 1; i < raw.size(); ++i) max_id = std::max({max_id, raw[i].first, raw[i].second});
        const std::uint64_t rem = raw.size() - 1;
        const bool count_ok = cm == rem || (cm > 0 && std::llabs(static_cast<long long>(cm) - static_cast<long long>(rem)) <= static_cast<long long>(std::max<std::uint64_t>(1, cm / 100)));
        const bool ids_ok = cn >= max_id;
        header = count_ok && ids_ok && cn > 1;
    }
    if (header) {
        ps.header_detected = true;
        declared_n = raw.front().first;
        begin = 1;
    }

    std::uint64_t min_id = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t max_id = 0;
    std::vector<std::uint64_t> ids;
    ids.reserve((raw.size() - begin) * 2);
    for (std::size_t i = begin; i < raw.size(); ++i) {
        min_id = std::min({min_id, raw[i].first, raw[i].second});
        max_id = std::max({max_id, raw[i].first, raw[i].second});
        ids.push_back(raw[i].first);
        ids.push_back(raw[i].second);
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    enum class IdMode { PreserveZero, ShiftOne, Compress };
    IdMode mode;
    std::uint64_t n64 = 0;
    if (header && max_id < declared_n) {
        mode = IdMode::PreserveZero; n64 = declared_n; ps.id_mode = "preserve_zero_based";
    } else if (header && min_id >= 1 && max_id <= declared_n) {
        mode = IdMode::ShiftOne; n64 = declared_n; ps.id_mode = "shift_one_based";
    } else if (!header && min_id == 0 && max_id <= ids.size() * 2ULL) {
        mode = IdMode::PreserveZero; n64 = max_id + 1; ps.id_mode = "preserve_zero_based";
    } else if (!header && min_id == 1 && max_id == ids.size()) {
        mode = IdMode::ShiftOne; n64 = max_id; ps.id_mode = "shift_one_based";
    } else {
        mode = IdMode::Compress; n64 = ids.size(); ps.id_mode = "compress_sorted_ids";
    }
    if (n64 > std::numeric_limits<Vertex>::max()) throw std::runtime_error("too many vertices");

    std::unordered_map<std::uint64_t, Vertex> map;
    if (mode == IdMode::Compress) {
        map.reserve(ids.size() * 2);
        for (std::size_t i = 0; i < ids.size(); ++i) map.emplace(ids[i], static_cast<Vertex>(i));
    }
    auto conv = [&](std::uint64_t x) -> Vertex {
        if (mode == IdMode::PreserveZero) return static_cast<Vertex>(x);
        if (mode == IdMode::ShiftOne) return static_cast<Vertex>(x - 1);
        return map.at(x);
    };

    std::vector<Edge> edges;
    edges.reserve(raw.size() - begin);
    for (std::size_t i = begin; i < raw.size(); ++i) {
        Vertex u = conv(raw[i].first), v = conv(raw[i].second);
        if (u == v) { ++ps.self_loops_removed; continue; }
        if (u > v) std::swap(u, v);
        edges.emplace_back(u, v);
    }
    std::sort(edges.begin(), edges.end());
    const auto before = edges.size();
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
    ps.duplicates_removed = before - edges.size();
    return make_csr(static_cast<Vertex>(n64), std::move(edges));
}

inline void run_greedy_mis(const CSRGraph& g, const std::vector<Vertex>& order,
                           std::vector<std::int8_t>& state) {
    std::fill(state.begin(), state.end(), std::int8_t{-1});
    for (Vertex v : order) {
        if (state[v] != -1) continue;
        state[v] = 1;
        for (std::uint64_t i = g.offsets[v]; i < g.offsets[v + 1]; ++i) {
            const Vertex w = g.neighbors[i];
            if (state[w] == -1) state[w] = 0;
        }
    }
}

} // namespace twbench
