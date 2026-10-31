# Chapter 4: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch04/`.

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
| `SelfFinancing/12` | 119 ns | 161 ns | 1.36 | 1,054 | 876 | 0.83 |
| `SelfFinancing/252` | 2.04 µs | 2.47 µs | 1.21 | 10,750 | 11,351 | 1.06 |
| `SelfFinancing/2520` | 22 µs | 23.7 µs | 1.08 | 97,271 | 106,812 | 1.10 |
| `SelfFinancingMulti/12` | 297 ns | 308 ns | 1.04 | 2,364 | 3,010 | 1.27 |
| `SelfFinancingMulti/252` | 5.86 µs | 6.05 µs | 1.03 | 38,220 | 56,298 | 1.47 |
| `SelfFinancingMulti/2520` | 60.8 µs | 60 µs | 0.99 | 371,953 | 552,725 | 1.49 |
| `Gains/12` | 6.46 ns | 10.6 ns | 1.65 | 132 | 184 | 1.39 |
| `Gains/252` | 173 ns | 221 ns | 1.28 | 2,172 | 3,184 | 1.47 |
| `Gains/2520` | 1.67 µs | 1.72 µs | 1.03 | 21,450 | 31,534 | 1.47 |
| `DiscountedGains/12` | 14.4 ns | 15.3 ns | 1.06 | 117 | 151 | 1.29 |
| `DiscountedGains/252` | 313 ns | 284 ns | 0.91 | 1,917 | 2,551 | 1.33 |
| `DiscountedGains/2520` | 2.61 µs | 3.2 µs | 1.23 | 18,927 | 25,231 | 1.33 |
| `IsArbitrageFree/2` | 1.48 ns | 1.27 ns | 0.86 | 24 | 31 | 1.29 |
| `IsArbitrageFree/3` | 2.72 ns | 1.78 ns | 0.65 | 37 | 42 | 1.14 |
| `IsArbitrageFree/100` | 102 ns | 75.2 ns | 0.74 | 761 | 624 | 0.82 |
| `ArbitrageStrategy/2` | 1.06 ns | 1.42 ns | 1.34 | 22 | 41 | 1.86 |
| `ArbitrageStrategy/3` | 1.43 ns | 2.36 ns | 1.66 | 29 | 50 | 1.72 |
| `ArbitrageStrategy/100` | 72.4 ns | 102 ns | 1.42 | 708 | 740 | 1.05 |
| `RiskNeutralProbs/2` | 1.24 ns | 7.95 ns | 6.42 | 26 | 50 | 1.92 |
| `RiskNeutralProbs/3` | 22.2 ns | 25 ns | 1.13 | 372 | 347 | 0.93 |
| `RiskNeutralProbs/100` | 408 ns | 371 ns | 0.91 | 3,007 | 2,575 | 0.86 |
| `OneStepValues/2` | 29.7 ns | 10.1 ns | 0.34 | 453 | 182 | 0.40 |
| `OneStepValues/3` | 50.3 ns | 10.1 ns | 0.20 | 685 | 190 | 0.28 |
| `OneStepValues/100` | 230 ns | 22 ns | 0.10 | 3,230 | 496 | 0.15 |
| `ExpectedDiscountedValue/4` | 174 ns | 210 ns | 1.21 | 3,216 | 3,401 | 1.06 |
| `ExpectedDiscountedValue/8` | 3.94 µs | 3.42 µs | 0.87 | 54,612 | 49,648 | 0.91 |
| `ExpectedDiscountedValue/12` | 138 µs | 155 µs | 1.12 | 947,352 | 876,435 | 0.93 |
| `ExpectedDiscountedValue/16` | 2.27 ms | 3.04 ms | 1.34 | 16,021,212 | 15,617,462 | 0.97 |
| `PrintReport` | 7.68 µs | 12.3 µs | 1.61 | 128,953 | 155,382 | 1.20 |

## Observations

- **`OneStepValues` is 3–10x faster in Rust.** The C++ `push_back`s into an
  empty vector; the Rust version maps over the returns, so the vector is
  allocated once at its final size and LLVM vectorises the loop. A `reserve`
  in the C++ would close most of the gap (left for a separate PR).
- **`Gains` and `DiscountedGains` run 1.3–1.5x the instructions, at about the
  same speed.** GCC vectorises the per-step terms two at a time with SSE2 and
  then adds them in order; LLVM keeps the loop scalar, because it will not
  vectorise a floating-point sum without permission to reorder it. The first
  port indexed `s[n]`, `x[n - 1]` and so on and ran 2.0–2.7x the
  instructions, because every index was bounds-checked; walking
  `windows(2)` and `zip` removed the checks.
- **The scenario scans (`IsArbitrageFree`, `ArbitrageStrategy`,
  `RiskNeutralProbs`) were 2.3–3.6x the instructions in the first port.**
  A shared `first_min_max` helper reloaded `values[lo]` and `values[hi]`
  (with bounds checks) on every element, and `ArbitrageStrategy` scanned for
  both extremes where the C++ stops after the maximum. Keeping the running
  extremes in registers and mirroring the C++ order brought the 100-scenario
  cases to 0.82–1.05x. At two or three scenarios the functions are 20–50
  instructions and the difference is fixed call overhead: the Rust
  functions save five callee-saved registers and return
  `Option<Vec<f64>>` through memory.
- **`SelfFinancingMulti` runs 1.5x the instructions in the same time.** The
  prices are `Vec<Vec<f64>>`, and each `x[j][n - 1]`, `s[j][n]` is still
  bounds-checked; iterating the stocks with `zip` removed the outer checks
  (from 1.77x). The checks are well predicted, so the time is unchanged.
- **`ExpectedDiscountedValue`** (the depth-first walk over 2^N paths, calling
  the strategy through `&dyn Fn` like the C++ `std::function`) is within
  ±10% in instructions. The time ratio at N = 16 (1.3x) varied between runs.
- **`PrintReport`** is 1.2x the instructions and 1.6x the time; the cost is
  Rust's `fmt` machinery, as in chapters 1–2.
