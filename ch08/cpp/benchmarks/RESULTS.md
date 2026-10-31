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

## After: log-space binomial weights, power tables, rolling CRR price

- **Overflow fixed in all three languages.** `BinomialSum` and
  `EuropeanPriceSum` (so also `CrrCall`) build the weights
  `C(N,k) p^k (1-p)^(N-k)` in log space. New tests at N = 2000 check that the
  weights sum to 1, that the CRR formula equals the sum, and that both are
  within 0.01 of Black-Scholes.
- `OptionTree` and `EarlyExerciseNodes` (C++) read node prices from two
  tables of powers (`StockPrices`, the same arithmetic as `StockPrice`, so the
  values are bit-identical) instead of two `std::pow` calls per node.
- `CrrTreePrice` (C++) keeps one row instead of the whole tree and calls the
  payoff directly instead of through `std::function`.

The report is byte-identical in C++, Kotlin and Python. Times at N = 2000 are
from a 5-repetition rerun (the coefficient of variation was 5-9%).

| Benchmark | Time before | Time after | Ir before | Ir after | Ir ratio |
|---|---:|---:|---:|---:|---:|
| `BM_OptionTreeAmerican/100` | 128 us | 27 us | 1,836,006 | 292,635 | 6.3x |
| `BM_OptionTreeAmerican/2000` | 55 ms | 7.7 ms | 659,362,841 | 95,187,021 | 6.9x |
| `BM_OptionTreeEuropean/2000` | 4.4 ms | 3.3 ms | 50,429,636 | 44,152,837 | 1.1x |
| `BM_EarlyExerciseNodes/2000` | 115 ms | 23 ms | 1,292,579,279 | 176,995,173 | 7.3x |
| `BM_CrrTreePriceEuropean/2000` | 4.2 ms | 1.3 ms | 50,649,123 | 10,650,616 | 4.8x |
| `BM_CrrTreePriceAmerican/2000` | 56 ms | 1.3 ms | 651,578,709 | 23,665,621 | 28x |
| `BM_EuropeanPriceSum/1000` | 52 us | 42 us | 603,989 | 514,981 | 1.2x |
| `BM_BinomialSum/1000` | 13 us | 15 us | 156,733 | 206,963 | 0.76x |
| `BM_CrrCall/1000` | 44 us | 43 us | 459,997 | 560,105 | 0.82x |
| `BM_PrintReport` | 20 ms | 1.4 ms | 245,971,630 | 15,475,242 | 16x |

`BinomialSum` and `CrrCall` cost about 20% more (an `exp` and two `log`s per
term instead of two `pow`s), and in exchange give correct results past
N = 1030 instead of `nan`.
