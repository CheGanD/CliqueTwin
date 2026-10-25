#include "../src/CliqueTwinCore.hpp"
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>

using namespace twbench;

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        std::cerr << "usage: ct_query_benchmark <graph> [queries=1000000] [seed=1]\n";
        return 1;
    }
    try {
        const std::uint64_t qn = argc >= 3 ? std::stoull(argv[2]) : 1000000ULL;
        const std::uint64_t seed = argc >= 4 ? std::stoull(argv[3]) : 1ULL;
        ParseStats ps;
        const CSRGraph g = read_graph(argv[1], ps);
        const CliqueTwinIndex ct = build_clique_twin(g);
        SplitMix64Rng rng(seed);

        std::uint64_t direct_edge=0, direct_nonedge=0, fallback=0, errors=0;
        const auto t0 = Clock::now();
        for (std::uint64_t i=0; i<qn; ++i) {
            Vertex u, v;
            do { u=static_cast<Vertex>(rng()%g.n); v=static_cast<Vertex>(rng()%g.n); } while (u==v);
            const Decision d=ct.decide(u,v);
            if (d==Decision::Edge) ++direct_edge;
            else if (d==Decision::NonEdge) ++direct_nonedge;
            else ++fallback;
            const bool answer = d==Decision::Edge ? true : d==Decision::NonEdge ? false : ct.residual.has_edge(u,v);
            if (answer != g.has_edge(u,v)) ++errors;
        }
        const double ns = std::chrono::duration<double,std::nano>(Clock::now()-t0).count()/double(qn);
        std::cout << std::fixed << std::setprecision(6)
                  << "queries="<<qn<<" direct_edge="<<direct_edge<<" direct_nonedge="<<direct_nonedge
                  << " fallback="<<fallback<<" fallback_pct="<<(100.0*double(fallback)/double(qn))
                  << " ns_per_query="<<ns<<" errors="<<errors<<"\n";
        return errors ? 3 : 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
}
