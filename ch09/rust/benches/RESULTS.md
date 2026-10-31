# Chapter 9: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch09/`.

**Machine:** cloud VM, 4 vCPU Intel Xeon at 2.1 GHz (shared). C++: GCC 13.3,
`-O3` Release, Google Benchmark 1.9.1, median of 5 repetitions. Rust: rustc
1.97, release profile (`opt-level = 3`, one codegen unit, fat LTO), Criterion
0.8, median of 100 samples. Instruction counts (Ir per iteration) come from
Valgrind 3.22 callgrind for both, as (Ir(N2) − Ir(N1)) / (N2 − N1), so set-up
cancels. Ratios below 1 mean Rust is faster or runs fewer instructions.

Wall-clock times on this VM vary by up to 10% run to run; the Ir counts are
exact. Random inputs are drawn from the same ranges in both languages but by
different generators (`std::mt19937_64` in C++, SplitMix64 in Rust).

| Benchmark | C++ time | Rust time | Rust/C++ | C++ Ir | Rust Ir | Rust/C++ |
|---|---:|---:|---:|---:|---:|---:|
| `NormCdf` | 13.5 ns | 10.9 ns | 0.81 | 128 | 117 | 0.92 |
| `NormPdf` | 7.26 ns | 8.99 ns | 1.24 | 77 | 72 | 0.94 |
| `NormInv` | 28.6 ns | 30 ns | 1.05 | 253 | 226 | 0.89 |
| `BsPriceCall` | 36.8 ns | 38 ns | 1.03 | 395 | 378 | 0.96 |
| `BsPricePut` | 41.3 ns | 30.7 ns | 0.74 | 401 | 380 | 0.95 |
| `BsGreeksCall` | 47.5 ns | 45 ns | 0.95 | 509 | 471 | 0.93 |
| `BsGreeksPut` | 55.1 ns | 52.1 ns | 0.94 | 631 | 586 | 0.93 |
| `TotalGreek/1` | 14.9 ns | 15.6 ns | 1.05 | 161 | 160 | 0.99 |
| `TotalGreek/10` | 141 ns | 146 ns | 1.03 | 1,490 | 1,381 | 0.93 |
| `TotalGreek/100` | 1.54 µs | 1.15 µs | 0.75 | 15,083 | 13,894 | 0.92 |
| `DeltaHedge/1` | 54.6 ns | 55.4 ns | 1.02 | 816 | 778 | 0.95 |
| `DeltaHedge/10` | 601 ns | 588 ns | 0.98 | 6,186 | 5,827 | 0.94 |
| `DeltaHedge/100` | 6.13 µs | 5.7 µs | 0.93 | 60,449 | 56,838 | 0.94 |
| `DeltaGammaHedge/1` | 218 ns | 172 ns | 0.79 | 1,924 | 1,938 | 1.01 |
| `DeltaGammaHedge/10` | 891 ns | 1.11 µs | 1.25 | 8,725 | 9,976 | 1.14 |
| `DeltaGammaHedge/100` | 8.28 µs | 9.53 µs | 1.15 | 77,682 | 91,185 | 1.17 |
| `PortfolioValue/1` | 36.4 ns | 30.7 ns | 0.84 | 415 | 403 | 0.97 |
| `PortfolioValue/10` | 437 ns | 373 ns | 0.85 | 3,983 | 3,842 | 0.96 |
| `PortfolioValue/100` | 3.95 µs | 3.66 µs | 0.93 | 39,659 | 38,225 | 0.96 |
| `RebalancePath/52` | 33.9 µs | 31.6 µs | 0.93 | 401,668 | 312,086 | 0.78 |
| `RebalancePath/252` | 176 µs | 146 µs | 0.83 | 1,901,697 | 1,471,448 | 0.77 |
| `Quantile` | 195 ns | 35.1 ns | 0.18 | 283 | 281 | 0.99 |
| `Cdf` | 20.8 ns | 19.7 ns | 0.95 | 177 | 167 | 0.94 |
| `ExpectedCallPayoff` | 34.7 ns | 35.5 ns | 1.02 | 338 | 282 | 0.83 |
| `ExpectedValue` | 55.2 ns | 56.5 ns | 1.02 | 677 | 879 | 1.30 |
| `InitialCost` | 57.1 ns | 63.2 ns | 1.11 | 627 | 748 | 1.19 |
| `ValueAtRisk` | 58.1 ns | 115 ns | 1.97 | 680 | 1,171 | 1.72 |
| `ExpectedReturn` | 122 ns | 148 ns | 1.21 | 1,374 | 1,637 | 1.19 |
| `PrintReport` | 70.9 µs | 91.6 µs | 1.29 | 748,021 | 935,799 | 1.25 |

## Observations

- **`TotalGreek` is 0.92x the C++ instructions; the first port ran 3.6x.**
  The benchmark asks for one Greek of a book. GCC inlines `BsGreeks` into
  `TotalGreek`, sees the constant `Greek`, and drops the `norm_cdf` and
  `exp` calls of the four Greeks nobody reads. LLVM kept `bs_greeks` out of
  line, so every option paid for all five. `#[inline(always)]` on
  `bs_greeks` and `total_greek` gives LLVM the same view (see STYLE.md).
- **`ValueAtRisk`, `ExpectedValue`, `InitialCost` and `ExpectedReturn` run
  1.2–1.7x the C++ instructions.** Each prices the legs of a strategy, and
  GCC inlines `BsPrice` and `ExpectedCallPayoff` so that legs with the same
  expiry share their `exp` and `log` calls. LLVM leaves those functions out
  of line and calls `exp` once per leg. The gap is a few hundred instructions
  per call; forcing more inlining was not worth the code size.
- **`RebalancePath` is 0.77x.** The Rust loop reuses one buffer for the
  shifted book on every step; the C++ allocates a new vector each step.
- **`DeltaGammaHedge` is 1.14–1.17x** for books of 10 and more: the hedge
  sums two Greeks per position. I did not isolate where the extra
  instructions come from.
- **`NormInv` (Acklam plus one Halley step) and the Black-Scholes formulas**
  are 0.89–0.96x the instructions; both call the same glibc `exp`, `log` and
  `erfc`. The C++ `Quantile` time (195 ns) is a noisy run; its instruction
  count is equal.
- **`PrintReport`** is 1.25x the instructions: the report is mostly
  formatting, where `fmt` costs more than `std::ostream` with fixed
  precision.
