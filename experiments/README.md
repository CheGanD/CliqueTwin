# Experiments

This directory contains compact CliqueTwin-specific diagnostics that can be run directly on any supported graph:

- `ct_structure.cpp`: clique offloading, residual graph size, PID/GT' allocation, and original/residual degeneracy.
- `ct_query_benchmark.cpp`: exact CliqueTwin edge-query routing and fallback rate on a deterministic uniform-pair workload, with correctness checked against the original graph.

These programs use the same `CliqueTwinCore.hpp` implementation as the examples and Triangle application.
