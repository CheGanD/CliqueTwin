# Datasets

CliqueTwin experiments use the following 15 real-world graphs. The datasets are not redistributed in this repository; download them from the original public collections.

| Dataset | Abbr. | Type | Source |
|---|---:|---|---|
| Gnutella | Gn | P2P | [SNAP](https://snap.stanford.edu/data/p2p-Gnutella31.html) |
| Epinions | Ep | Social | [SNAP](https://snap.stanford.edu/data/soc-Epinions1.html) |
| Slashdot0902 | Sl | Social | [SNAP](https://snap.stanford.edu/data/soc-Slashdot0902.html) |
| Stanford | St | Web | [SNAP](https://snap.stanford.edu/data/web-Stanford.html) |
| NotreDame | No | Web | [SNAP](https://snap.stanford.edu/data/web-NotreDame.html) |
| BerkStan | Be | Web | [SNAP](https://snap.stanford.edu/data/web-BerkStan.html) |
| Google | Go | Web | [SNAP](https://snap.stanford.edu/data/web-Google.html) |
| Youtube | Yo | Social | [SNAP](https://snap.stanford.edu/data/com-Youtube.html) |
| Wikitalk | Wt | Communication | [SNAP](https://snap.stanford.edu/data/wiki-Talk.html) |
| LiveJournal | Li | Social | [SNAP](https://snap.stanford.edu/data/soc-LiveJournal1.html) |
| CTR | CTR | Road | [9th DIMACS Implementation Challenge](https://www.diag.uniroma1.it/challenge9/download.shtml) |
| USA | USA | Road | [9th DIMACS Implementation Challenge](https://www.diag.uniroma1.it/challenge9/download.shtml) |
| IT-2004 | IT | Web crawl | [LAW](https://law.di.unimi.it/datasets.php) |
| SK | SK | Web crawl | [LAW](https://law.di.unimi.it/datasets.php) |
| Friendster | Fr | Social | [SNAP](https://snap.stanford.edu/data/com-Friendster.html) |

## Input format

The C++ reader accepts an undirected edge list with optional comment lines and an optional `n m` header. Vertex IDs may be zero-based, one-based, or sparse integer IDs; sparse IDs are compacted internally. Self-loops and duplicate undirected edges are removed.

For reproducible experiments, keep one local file per dataset and record the exact downloaded archive/version and checksum before preprocessing.
