# Chapter 8 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`./bench.sh --callgrind` from `ch08/` after a change and compare. The
callgrind pass takes several minutes because of the 2000-step American cases.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are means of repeated runs; on this
VM they vary by up to about 10%, so compare versions by instruction count (Ir),
which is exact and repeatable.

One-year CRR trees, r = 5%, sigma = 20%, at the money (puts for the American
trees):

| Benchmark | N=10 | N=100 | N=500 | N=2000 | Ir at N=2000 |
|---|---:|---:|---:|---:|---:|
| `BM_OptionTreeEuropean` | 1.1 us | 25 us | 306 us | 4.4 ms | 50,429,636 |
| `BM_OptionTreeAmerican` | 2.0 us | 128 us | 3.7 ms | 55 ms | 659,362,841 |
| `BM_EarlyExerciseNodes` | 3.2 us | 276 us | 7.1 ms | 115 ms | 1,292,579,279 |
| `BM_CrrTreePriceEuropean` | 0.79 us | 25 us | 314 us | 4.2 ms | 50,649,123 |
| `BM_CrrTreePriceAmerican` | 2.2 us | 150 us | 3.4 ms | 56 ms | 651,578,709 |

| Benchmark | N=10 | N=100 | N=1000 | Ir at N=1000 |
|---|---:|---:|---:|---:|
| `BM_BinomialSum` | 140 ns | 1.5 us | 13 us | 156,733 |
| `BM_EuropeanPriceSum` | 559 ns | 5.7 us | 52 us | 603,989 |
| `BM_CrrCall` | 424 ns | 4.6 us | 44 us | 459,997 |

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_ReplicatingPosition` | 91 ns | 1,014 |
| `BM_NormCdf` | 12 ns | 128 |
| `BM_BsCall` | 42 ns | 394 |
| `BM_BsPut` | 42 ns | 396 |
| `BM_BsCallDelta` | 21 ns | 199 |
| `BM_PrintReport` (whole chapter, to memory) | 20 ms | 245,971,630 |

## Observations

- **Overflow:** `BinomialSum`, `EuropeanPriceSum` and therefore `CrrCall`
  build the binomial coefficient `C(N, k)` as a running product, which passes
  `DBL_MAX` near N = 1030. From there the sums return `nan`. For example,
  `CrrCall` at N = 1030 gives `nan` where the tree gives 10.4486. The report
  uses smaller N, so its output is unaffected; the benchmarks stop at 1000.
  Building the terms in log space, or as a ratio recurrence on the whole term
  `C(N,k) q^k (1-q)^(N-k)`, fixes it.
- American trees are about 13x slower than European, as in chapter 7:
  `OptionTree` recomputes the node's stock price with two `std::pow` calls at
  every node for the early-exercise check. `EarlyExerciseNodes` builds that
  tree and then does a second full pass with the same `pow` calls.
- `CrrTreePrice` returns only `[0][0]` but builds and keeps the full
  (N+1)(N+2)/2-node `Tree` (about 16 MB at N = 2000). A single rolling vector
  would do; chapter 7's `CrrPrice` does it that way and is about 3x faster at
  N = 2000 for European options.
- The payoff is a `std::function`, so each leaf is an indirect call. This is
  small next to the `pow` calls.
- `PrintReport` spends its 20 ms almost entirely in American trees.
