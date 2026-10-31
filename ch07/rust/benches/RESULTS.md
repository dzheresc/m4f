# Chapter 7: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch07/`.

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
| `CallPayoff` | 1.17 ns | 0.822 ns | 0.70 | 10 | 9 | 0.86 |
| `Profit` | 0.965 ns | 0.757 ns | 0.79 | 10 | 10 | 1.00 |
| `ParityGap` | 0.638 ns | 0.548 ns | 0.86 | 12 | 9 | 0.75 |
| `EuropeanBounds` | 2.01 ns | 1.43 ns | 0.71 | 19 | 22 | 1.17 |
| `AmericanParityBounds` | 0.585 ns | 0.754 ns | 1.29 | 14 | 11 | 0.79 |
| `ButterflyGap` | 1.52 ns | 1.25 ns | 0.82 | 21 | 20 | 0.95 |
| `CrrEuropeanCall/10` | 432 ns | 437 ns | 1.01 | 4,012 | 4,956 | 1.24 |
| `CrrEuropeanCall/100` | 6.11 µs | 5.91 µs | 0.97 | 56,974 | 62,169 | 1.09 |
| `CrrEuropeanCall/500` | 94.1 µs | 91.7 µs | 0.97 | 793,235 | 806,338 | 1.02 |
| `CrrEuropeanCall/2000` | 1.13 ms | 1.31 ms | 1.16 | 10,669,104 | 10,719,482 | 1.00 |
| `CrrEuropeanPut/10` | 413 ns | 432 ns | 1.05 | 4,018 | 4,960 | 1.23 |
| `CrrEuropeanPut/100` | 5.24 µs | 4.86 µs | 0.93 | 57,025 | 62,219 | 1.09 |
| `CrrEuropeanPut/500` | 89.5 µs | 92.2 µs | 1.03 | 793,486 | 806,556 | 1.02 |
| `CrrEuropeanPut/2000` | 1.24 ms | 1.08 ms | 0.87 | 10,670,105 | 10,720,451 | 1.00 |
| `CrrAmericanCall/10` | 421 ns | 430 ns | 1.02 | 4,454 | 5,099 | 1.14 |
| `CrrAmericanCall/100` | 5.85 µs | 5.95 µs | 1.02 | 79,511 | 74,867 | 0.94 |
| `CrrAmericanCall/500` | 93.3 µs | 91.3 µs | 0.98 | 1,305,972 | 1,119,805 | 0.86 |
| `CrrAmericanCall/2000` | 1.36 ms | 1.07 ms | 0.79 | 18,720,091 | 15,723,449 | 0.84 |
| `CrrAmericanPut/10` | 433 ns | 437 ns | 1.01 | 4,496 | 5,128 | 1.14 |
| `CrrAmericanPut/100` | 6.12 µs | 5.91 µs | 0.97 | 82,163 | 77,417 | 0.94 |
| `CrrAmericanPut/500` | 96.3 µs | 80.4 µs | 0.84 | 1,369,224 | 1,182,554 | 0.86 |
| `CrrAmericanPut/2000` | 1.29 ms | 1.35 ms | 1.04 | 19,723,093 | 16,724,449 | 0.85 |
| `PrintReport` | 593 µs | 753 µs | 1.27 | 7,257,299 | 7,079,814 | 0.98 |

## Observations

- **The American CRR trees take 0.84–0.86x the C++ instructions** at
  N = 500–2000 and about the same time. The first port ran 2.4x the C++
  instructions: its backward step indexed the row and the node-price table
  with bounds checks, and the price index `N - n + 2k` read every other
  table entry, so LLVM would not vectorise the loop the way GCC does. Two
  changes fixed it. The table is split by parity into two tables, so each
  row's prices are one contiguous slice. The in-place update
  `v[k] = f(v[k], v[k + 1])` walks `Cell::as_slice_of_cells().windows(2)`
  zipped with that slice, so there is no index left to check. Each payoff
  also gets its own copy of the loop (a generic closure), which is what GCC
  does by unswitching on `is_call`.
- **The European trees** are within ±2% of the C++ instructions from
  N = 500 up; they were already bounds-check free because the row is sliced
  to its length before the loop. At N = 10 the fixed set-up (two extra
  table allocations) costs about 20% more.
- **The option bound and parity functions** are 9–22 instructions each in
  both languages; the differences are a couple of instructions and the
  times are under 2 ns.
- **`PrintReport`** is 0.98x the instructions and 1.27x the time. The report
  prices 200-step trees, so the trees dominate the instruction count; the
  time difference is the formatting.
