#include "../src/CliqueTwinCore.hpp"
#include <iostream>
#include <string>

using namespace twbench;

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: ct_edge_query <graph> <u> <v>\n";
        return 1;
    }
    try {
        ParseStats ps;
        const CSRGraph g = read_graph(argv[1], ps);
        const Vertex u = static_cast<Vertex>(std::stoul(argv[2]));
        const Vertex v = static_cast<Vertex>(std::stoul(argv[3]));
        if (u >= g.n || v >= g.n || u == v) throw std::runtime_error("invalid query vertices");

        const CliqueTwinIndex ct = build_clique_twin(g);
        const Decision d = ct.decide(u, v);
        const bool edge = ct.edge_query(u, v);

        const char* route = d == Decision::Edge ? "PID_EDGE" :
                            d == Decision::NonEdge ? "GT_PRIME_NONEDGE" : "RESIDUAL_FALLBACK";
        std::cout << "route=" << route << " answer=" << (edge ? "EDGE" : "NONEDGE") << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
}
