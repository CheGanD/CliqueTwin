# CliqueTwin Triangle Counting

`triangle_ct_online.cpp` is the CliqueTwin triangle-counting path used in this repository. It builds CliqueTwin from the input graph and counts triangles in three disjoint parts:

1. triangles entirely inside an extracted clique;
2. triangles containing two vertices from one extracted clique and a third vertex outside that clique;
3. triangles in the residual graph.

Clique sizes and per-vertex neighbor PID counts are derived online from the generic CliqueTwin header/adjacency representation. Residual triangle counting uses the residual negative certificate as an aggregate skip before exact residual traversal.

Run:

```bash
./build/ct_triangle_online data/example.txt
```
