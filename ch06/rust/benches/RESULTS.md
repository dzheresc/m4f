# Chapter 6: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch06/`.

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
| `ForwardPrice` | 4.37 ns | 5.6 ns | 1.28 | 70 | 67 | 0.96 |
| `ForwardPriceYield` | 4.29 ns | 4.78 ns | 1.11 | 71 | 68 | 0.96 |
| `ForwardArbitrageProfit` | 4.71 ns | 3.95 ns | 0.84 | 73 | 69 | 0.95 |
| `LongForwardValue` | 4.43 ns | 4.75 ns | 1.07 | 71 | 67 | 0.94 |
| `ForwardPriceDividends/4` | 26.4 ns | 18.5 ns | 0.70 | 373 | 348 | 0.93 |
| `ForwardPriceDividends/100` | 436 ns | 465 ns | 1.07 | 7,285 | 6,612 | 0.91 |
| `FuturesTree/3` | 232 ns | 153 ns | 0.66 | 3,292 | 2,023 | 0.61 |
| `FuturesTree/10` | 988 ns | 419 ns | 0.42 | 13,119 | 5,928 | 0.45 |
| `FuturesTree/100` | 23.1 µs | 7.52 µs | 0.33 | 304,488 | 121,666 | 0.40 |
| `FuturesTree/1000` | 1.05 ms | 399 µs | 0.38 | 13,564,115 | 8,412,516 | 0.62 |
| `MarginAccount/252` | 1.86 µs | 890 ns | 0.48 | 8,129 | 4,940 | 0.61 |
| `MarginAccount/2520` | 20.2 µs | 8.62 µs | 0.43 | 106,309 | 45,769 | 0.43 |
| `MinVarianceHedgeRatio/3` | 4.59 ns | 4.63 ns | 1.01 | 105 | 92 | 0.88 |
| `MinVarianceHedgeRatio/100` | 132 ns | 161 ns | 1.22 | 1,997 | 2,035 | 1.02 |
| `MinVarianceHedgeRatio/10000` | 14.8 µs | 15.6 µs | 1.05 | 195,047 | 200,035 | 1.03 |
| `HedgedStd/3` | 57.5 ns | 13 ns | 0.23 | 785 | 277 | 0.35 |
| `HedgedStd/100` | 337 ns | 140 ns | 0.41 | 4,332 | 1,669 | 0.39 |
| `HedgedStd/10000` | 59.7 µs | 15 µs | 0.25 | 362,453 | 145,466 | 0.40 |
| `PrintReport` | 9.85 µs | 11.9 µs | 1.21 | 138,780 | 171,057 | 1.23 |

## Observations

- **`FuturesTree` is 0.33–0.66x the C++ time.** The C++ `push_back`s each
  row of the tree into an empty vector, so it reallocates as the row grows;
  the Rust version `collect`s each row from a range, which allocates once at
  the final size and lets LLVM vectorise the backward step. At N = 1000 the
  two `powf` calls per terminal node are the same in both languages.
- **`MarginAccount` and `HedgedStd` are 0.25–0.5x the time** for the same
  reason: the C++ grows its output with `push_back` and no `reserve`. A
  `reserve` in the C++ would close most of the gap (left for a separate PR).
- **`MinVarianceHedgeRatio`** was 1.46x the C++ instructions in the first
  port: its two loops indexed `ds[i]`, `df[i]` and `p[i]`, each with a bounds
  check. Slicing `ds` and `df` to `p.len()` once before the loops removed
  the checks and brought it to 1.02x. Its rows and `HedgedStd`'s were re-run
  after the change; the other rows are from the full run before it.
- **The forward price formulas** (`ForwardPrice`, `ForwardPriceYield`, ...)
  are one `exp` each and within ±6% in instructions; the times are a few
  nanoseconds and within run-to-run noise.
- **`PrintReport`** is 1.23x the instructions and 1.2x the time: Rust's
  `fmt` costs more than `std::format` per number.
