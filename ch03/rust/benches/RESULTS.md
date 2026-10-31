# Chapter 3: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch03/`.

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
| `Returns/252` | 383 ns | 161 ns | 0.42 | 6,385 | 1,761 | 0.28 |
| `Returns/2520` | 3.16 µs | 1.24 µs | 0.39 | 60,624 | 14,238 | 0.23 |
| `LogReturns/252` | 1.39 µs | 1.18 µs | 0.85 | 18,962 | 15,629 | 0.82 |
| `LogReturns/2520` | 12.9 µs | 12.2 µs | 0.95 | 186,621 | 152,845 | 0.82 |
| `CompoundReturn/252` | 262 ns | 218 ns | 0.83 | 1,524 | 904 | 0.59 |
| `CompoundReturn/2520` | 2.93 µs | 2.87 µs | 0.98 | 15,132 | 8,842 | 0.58 |
| `ExpectationVariance/252` | 439 ns | 438 ns | 1.00 | 3,720 | 3,823 | 1.03 |
| `ExpectationVariance/2520` | 3.85 µs | 4.66 µs | 1.21 | 36,606 | 37,843 | 1.03 |
| `BinomialProb/3` | 51.2 ns | 50.1 ns | 0.98 | 654 | 631 | 0.96 |
| `BinomialProb/10` | 63.1 ns | 57.3 ns | 0.91 | 755 | 732 | 0.97 |
| `BinomialProb/100` | 54.4 ns | 50.9 ns | 0.94 | 691 | 668 | 0.97 |
| `BinomialProb/1000` | 53.7 ns | 51.8 ns | 0.96 | 691 | 669 | 0.97 |
| `Price` | 26.6 ns | 25.5 ns | 0.96 | 289 | 279 | 0.97 |
| `RiskNeutralProb` | 1.12 ns | 1.2 ns | 1.08 | 14 | 13 | 0.93 |
| `ExpectedPrice/3` | 56 ns | 60.9 ns | 1.09 | 754 | 845 | 1.12 |
| `ExpectedPrice/10` | 161 ns | 160 ns | 0.99 | 2,189 | 2,275 | 1.04 |
| `ExpectedPrice/100` | 1.47 µs | 1.39 µs | 0.94 | 20,639 | 20,635 | 1.00 |
| `ExpectedPrice/1000` | 16.3 µs | 15.1 µs | 0.93 | 209,769 | 208,902 | 1.00 |
| `ConditionalDiscounted` | 78 ns | 62 ns | 0.79 | 863 | 712 | 0.83 |
| `TrinomialRiskNeutralProbs` | 1.34 ns | 1.24 ns | 0.93 | 21 | 18 | 0.86 |
| `NormCdf` | 7.15 ns | 7.68 ns | 1.07 | 99 | 92 | 0.93 |
| `BinomialExpectedPrice/10` | 24.8 ns | 27.2 ns | 1.10 | 278 | 270 | 0.97 |
| `BinomialExpectedPrice/100` | 25.5 ns | 26.1 ns | 1.02 | 278 | 270 | 0.97 |
| `BinomialExpectedPrice/1000` | 22.8 ns | 25.7 ns | 1.12 | 278 | 270 | 0.97 |
| `BinomialExpectedPrice/10000` | 25.4 ns | 26.2 ns | 1.03 | 278 | 270 | 0.97 |
| `LognormalExpectedPrice` | 5.32 ns | 4.74 ns | 0.89 | 72 | 72 | 1.00 |
| `BinomialProbAbove/10` | 121 ns | 96.7 ns | 0.80 | 1,673 | 1,632 | 0.98 |
| `BinomialProbAbove/100` | 1.22 µs | 1.14 µs | 0.93 | 16,943 | 16,586 | 0.98 |
| `BinomialProbAbove/1000` | 11.6 µs | 11.6 µs | 1.00 | 170,752 | 167,250 | 0.98 |
| `BinomialProbAbove/10000` | 124 µs | 117 µs | 0.94 | 1,730,448 | 1,695,496 | 0.98 |
| `LognormalProbAbove` | 7.38 ns | 8.01 ns | 1.09 | 101 | 92 | 0.91 |
| `PrintReport` | 167 µs | 141 µs | 0.84 | 2,117,650 | 2,043,614 | 0.97 |

## Observations

- **`Returns` is 2.5x faster in Rust** (0.42x the time, 0.23–0.28x the
  instructions). The C++ version `push_back`s into an empty vector, so it
  reallocates as it grows. The Rust version collects from `path.windows(2)`,
  which knows its exact length, so it allocates once and LLVM vectorises the
  divisions. A `reserve` in the C++ would close most of the gap. That is a
  C++ change, so it is left for a separate PR.
- **`LogReturns`** (0.82x Ir) gets the same single allocation, but the cost
  is dominated by `ln`, which both languages call in glibc.
- **`CompoundReturn`** runs 42% fewer instructions in Rust, yet the time is
  the same. The loop is a chain of dependent multiplications, so latency
  sets the speed, not the instruction count.
- **The log-space binomial sums (`ExpectedPrice`, `BinomialProbAbove`) are
  identical in cost** (±2% Ir at N = 100–10000) once their loops use
  half-open ranges. With `0..=n` (the first port) they had the same overhead
  as chapter 2's yield solver.
- **`BinomialProb`** calls glibc's `lgamma` three times in both languages.
  Rust reaches it through a one-line `extern "C"` declaration
  (`src/libm.rs`), because `f64::ln_gamma` is not stable yet. That is the
  crate's only `unsafe`, and the lint is `deny` with a single `allow` there.
- **`PrintReport`** is 0.84x the time. This chapter's report spends its time
  in `BinomialProbAbove` at N = 10000, not in formatting.
