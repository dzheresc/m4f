# C++ performance report

Every chapter's C++ code has Google Benchmark microbenchmarks
(`chNN/bench.sh`) and exact instruction counts from callgrind
(`chNN/bench.sh --callgrind`). Fixes were applied to ten chapters; chapter 6
needed none. Each chapter's hottest path now runs **9.5x to 248x fewer
instructions**. One correctness bug was found and fixed: binomial weights
overflowed past N ≈ 1030 in chapters 3 and 8.

Every change keeps each chapter's report byte-identical to its
`expected_output.txt`. Each change also has a unit test against the code it
replaced, bit-identical wherever the arithmetic allows.

## Method

- **Primary metric:** instructions per iteration (Ir) from callgrind,
  computed as (Ir(N2) − Ir(N1)) / (N2 − N1). The count is exact and
  repeatable; wall-clock times on the shared VM vary by up to 10%.
- **Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
  Google Benchmark 1.9.1, Valgrind 3.22. Times are medians of 5 repetitions.
- **Detail:** each `chNN/cpp/benchmarks/RESULTS.md` has the baseline, notes
  on each hot spot, and an "After" table.

## Headline per chapter

| Ch | Benchmark | Ir before | Ir after | Ratio | Time before → after | Change | PR |
|---:|---|---:|---:|---:|---|---|---|
| 1 | BestCycleScaling/8 | 311,063,588 | 3,292,582 | 94x | 26 ms → 188 µs | n×n best-leg index table, integer search | #34 |
| 2 | CouponBondPrice/360 | 51,272 | 207 | 248x | 4.7 µs → 29 ns | closed-form annuity (expm1/log1p) | #35 |
| 2 | YieldToMaturity/360 | 2,274,043 | 11,937 | 191x | 187 → 1.7 µs | Newton inside the bisection bracket | #35 |
| 3 | ExpectedPrice/1000 | 6,094,545 | 209,782 | 29x | 2.7 ms → 16 µs | O(n) log-space sum (overflow fix) | #36 |
| 4 | ExpectedDiscountedValue/16 | 853,216,750 | 16,021,212 | 53x | 88 → 2.8 ms | depth-first walk sharing prefixes | #37 |
| 5 | MinVariancePortfolio/200 | 66,458,312 | 6,977,037 | 9.5x | 3.8 → 0.64 ms | Cholesky solve, row-wise VecMat | #38 |
| 6 | — | | | | | no change needed | |
| 7 | CrrAmericanPut/2000 | 602,449,553 | 19,723,093 | 31x | 50 → 1.4 ms | power table instead of pow per node | #39 |
| 8 | CrrTreePriceAmerican/2000 | 651,578,709 | 23,665,621 | 28x | 56 → 1.3 ms | rolling row, power tables | #40 |
| 9 | NormInv | 27,273 | 253 | 108x | 5.3 µs → 31 ns | Acklam + one Halley step | #41 |
| 10 | YieldToMaturity/3 | 52,344 | 952 | 55x | 5.2 µs → 105 ns | Newton inside the bisection bracket | #42 |
| 11 | BondPrices/200 | 131,899,524 | 1,752,391 | 75x | 10 ms → 155 µs | forward induction, O(N²) instead of O(N³) | #43 |

## Other notable results

| Ch | Benchmark | Ir before | Ir after | Ratio | Time before → after |
|---:|---|---:|---:|---:|---|
| 1 | BestCycleBook | 13,263 | 3,315 | 4.0x | 959 → 236 ns |
| 3 | BinomialProb/1000 | 5,817 | 691 | 8.4x | 2.5 µs → 62 ns |
| 4 | SelfFinancing/12 | 3,990 | 1,054 | 3.8x | 295 → 117 ns |
| 4 | PrintReport | 322,747 | 128,953 | 2.5x | 22 → 11 µs |
| 5 | VecMat/200 | 1,269,144 | 164,137 | 7.7x | 112 → 11 µs |
| 5 | Correlation/10000 | 545,145 | 250,076 | 2.2x | 55 → 22 µs |
| 5 | PrintReport | 220,540 | 167,573 | 1.3x | 16 → 13 µs |
| 7 | PrintReport | 82,435,899 | 7,257,299 | 11x | 7.5 → 0.75 ms |
| 8 | EarlyExerciseNodes/2000 | 1,292,579,279 | 176,995,173 | 7.3x | 115 → 23 ms |
| 8 | OptionTreeAmerican/2000 | 659,362,841 | 95,187,021 | 6.9x | 55 → 7.7 ms |
| 8 | PrintReport | 245,971,630 | 15,475,242 | 16x | 20 → 1.4 ms |
| 9 | ValueAtRisk | 25,911 | 680 | 38x | 1.9 µs → 63 ns |
| 9 | PrintReport | 1,484,238 | 748,021 | 2.0x | 172 → 71 µs |
| 10 | CouponBondPrice/360 | 392,058 | 10,089 | 39x | 47 → 0.80 µs |
| 10 | DynamicHedge/260 | 1,389,867 | 289,974 | 4.8x | 103 → 20 µs |
| 10 | PrintReport | 684,803 | 500,339 | 1.4x | 55 → 40 µs |
| 11 | Caplet/1000 | 61,141,331 | 39,119,175 | 1.6x | 4.6 → 2.6 ms |
| 11 | RateLattice/1000 | 14,897,152 | 7,152,194 | 2.1x | 2.7 → 1.7 ms |

Three benchmarks got slower, each on purpose:

- Ch 1 `FxMarket` construction: 0.55x. It builds the best-leg table once per market.
- Ch 3 `BinomialProb/3`: 0.48x. `lgamma` costs more than a tiny product.
- Ch 8 `BinomialSum` and `CrrCall` at N = 1000: 0.76x and 0.82x. They now use
  log-space weights, which stay correct past N ≈ 1030.

## Correctness bug fixed

The weights C(N, k) p^k (1−p)^(N−k) were computed directly. Past N ≈ 1030,
C(N, k) overflows a double to infinity and the sums became NaN. The affected
functions were `BinomialProb` and `ExpectedPrice` in chapter 3, and
`BinomialSum`, `EuropeanPriceSum` and `CrrCall` in chapter 8. All three
languages now use log-space weights (`lgamma` in C++ and Python, a sum of logs
in Kotlin), with large-N tests in each.

## Left alone

- **Chapter 6:** nothing was hot.
- **Chapter 11 lattice functions:** their cost is one `exp` per node for the
  discount factor. Removing it would put a precomputed discount lattice in the
  public API.
- **Kotlin and Python:** only the bug fixes were ported. The speedups are in
  C++ only.

## Pull requests

Benchmark harness: #22 and #24 to #33, one per chapter. Fixes: #34 to #43.
