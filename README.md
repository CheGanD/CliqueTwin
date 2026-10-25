# CliqueTwin

Public artifact repository for the PVLDB/VLDB 2027 submission:

**CliqueTwin: Eliminating Tail Fallbacks in Cache-Centric Graph Representations via Dual Certificates**

## Artifact scope

This repository is intentionally limited to the experiments supporting the paper's main claims:

1. GT64 long-tail concentration under uniform edge queries.
2. Structural diagnosis: degeneracy/core effects, same-bit residualization, and matched-random control.
3. PID / residual-GT bit-budget allocation under a fixed 64-bit header.
4. GT64/GT128/GT256 vs. CliqueTwin-64 systems evaluation, including fallback, latency, LLC misses, mixed edge/non-edge queries, and boundary cases.
5. End-to-end triangle counting.
6. End-to-end subgraph matching (GPM).

Exploratory, superseded, or debugging-only experiments are intentionally excluded.

## Current public snapshot

This initial snapshot contains:
- the public repository layout;
- paper-facing experiment descriptions;
- processed numerical results already frozen for the paper;
- dataset provenance instructions.

The final source code, runners, and frozen raw-result bundles will be added before the full-paper submission deadline. No placeholder code is presented as a completed artifact.

## Repository layout

```text
experiments/
  01_long_tail/
  02_structural_mechanism/
  03_pid_budget/
  04_tail_cache/
applications/
  triangle/
  subgraph_matching/
data/
results/processed/
figures/data/
scripts/
```

## Datasets

Large public graph datasets are not redistributed in this repository. `data/README.md` records the expected public-source/provenance workflow.

## Reproducibility

Every final experiment directory will contain:
- exact build/run commands;
- configuration and seeds;
- expected output schema;
- paper-facing processed tables;
- checksums for frozen result bundles where applicable.

## Contact

Please use the author contact information provided in the paper submission.
