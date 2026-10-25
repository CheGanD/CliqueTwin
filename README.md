# CliqueTwin

CliqueTwin is a fixed-footprint graph representation for exact edge queries. It combines a positive clique certificate with a residual negative certificate in one 64-bit header per vertex.

For a pair of vertices `(u, v)`, CliqueTwin follows three steps:

1. **Positive certificate:** equal nonzero PIDs certify an edge inside an extracted clique.
2. **Negative certificate:** a shared residual GT' bit certifies a non-edge.
3. **Exact fallback:** unresolved pairs are checked in the residual adjacency structure.

The representation is exact: certificate decisions are one-sided, and unresolved pairs fall back to exact lookup.

## Construction

The implementation in `src/CliqueTwinCore.hpp`:

1. greedily extracts vertex-disjoint cliques, starting at size 5 and supplementing with sizes 4 and 3 when PID capacity permits;
2. assigns each extracted clique a nonzero PID;
3. removes clique-internal edges to form a residual graph;
4. builds an MIS-based negative certificate on the residual graph;
5. packs PID bits and residual GT' bits into a 64-bit vertex header.

## Repository layout

```text
src/
  GraphUtils.hpp          graph I/O, CSR, and generic MIS utilities
  CliqueTwinCore.hpp      CliqueTwin construction and exact query path
examples/
  edge_query.cpp          minimal edge-query example
applications/triangle/
  triangle_ct_online.cpp  CliqueTwin triangle counting
experiments/
  ct_structure.cpp        residual-structure statistics
  ct_query_benchmark.cpp  CliqueTwin query/fallback benchmark
data/
  README.md               datasets and download sources
  datasets.csv            machine-readable dataset list
  example.txt             small local test graph
```

## Build

Requirements:

- C++17 compiler
- CMake 3.16 or newer

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Or run:

```bash
./scripts/build.sh
```

## Quick start

Build CliqueTwin and answer one edge query:

```bash
./build/ct_edge_query data/example.txt 0 1
```

Inspect the residual structure:

```bash
./build/ct_structure data/example.txt
```

Run a query benchmark:

```bash
./build/ct_query_benchmark data/example.txt 100000 1
```

Run CliqueTwin triangle counting:

```bash
./build/ct_triangle_online data/example.txt
```

## Datasets

The main experiments use 15 real-world graphs: Gnutella, Epinions, Slashdot0902, Stanford, NotreDame, BerkStan, Google, Youtube, Wikitalk, LiveJournal, CTR, USA, IT-2004, SK, and Friendster.

The graph files are not included. See [`data/README.md`](data/README.md) for the original public download sources and expected input format.
