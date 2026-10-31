# Chapter 8: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch08/`.

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
| `OptionTreeEuropean/10` | 710 ns | 497 ns | 0.70 | 8,490 | 6,612 | 0.78 |
| `OptionTreeEuropean/100` | 12.7 µs | 9.52 µs | 0.75 | 163,336 | 129,406 | 0.79 |
| `OptionTreeEuropean/500` | 208 µs | 184 µs | 0.88 | 3,105,130 | 2,441,456 | 0.79 |
| `OptionTreeEuropean/2000` | 14.1 ms | 7.36 ms | 0.52 | 44,152,837 | 34,002,021 | 0.77 |
| `OptionTreeAmerican/10` | 747 ns | 560 ns | 0.75 | 10,052 | 7,999 | 0.80 |
| `OptionTreeAmerican/100` | 24.1 µs | 13.1 µs | 0.54 | 292,635 | 255,758 | 0.87 |
| `OptionTreeAmerican/500` | 624 µs | 372 µs | 0.60 | 6,301,268 | 5,573,195 | 0.88 |
| `OptionTreeAmerican/2000` | 19.8 ms | 6.02 ms | 0.30 | 95,187,021 | 84,029,023 | 0.88 |
| `EarlyExerciseNodes/10` | 1.38 µs | 1.16 µs | 0.84 | 17,114 | 14,817 | 0.87 |
| `EarlyExerciseNodes/100` | 48.1 µs | 30.7 µs | 0.64 | 534,073 | 504,558 | 0.94 |
| `EarlyExerciseNodes/500` | 1.23 ms | 642 µs | 0.52 | 11,529,282 | 11,139,204 | 0.97 |
| `EarlyExerciseNodes/2000` | 29.3 ms | 8.68 ms | 0.30 | 176,995,173 | 171,920,452 | 0.97 |
| `ReplicatingPosition` | 97.4 ns | 63.5 ns | 0.65 | 1,018 | 766 | 0.75 |
| `BinomialSum/10` | 186 ns | 121 ns | 0.65 | 2,081 | 1,793 | 0.86 |
| `BinomialSum/100` | 1.63 µs | 1.17 µs | 0.72 | 20,531 | 17,003 | 0.83 |
| `BinomialSum/1000` | 13.2 µs | 11.6 µs | 0.88 | 206,963 | 170,069 | 0.82 |
| `EuropeanPriceSum/10` | 518 ns | 413 ns | 0.80 | 5,696 | 5,217 | 0.92 |
| `EuropeanPriceSum/100` | 4.47 µs | 3.9 µs | 0.87 | 51,821 | 48,597 | 0.94 |
| `EuropeanPriceSum/1000` | 43.4 µs | 30.3 µs | 0.70 | 514,981 | 484,308 | 0.94 |
| `CrrCall/10` | 510 ns | 423 ns | 0.83 | 6,621 | 5,711 | 0.86 |
| `CrrCall/100` | 4.89 µs | 3.99 µs | 0.82 | 56,616 | 52,422 | 0.93 |
| `CrrCall/1000` | 47.7 µs | 35.1 µs | 0.74 | 560,105 | 486,618 | 0.87 |
| `NormCdf` | 11 ns | 7.84 ns | 0.72 | 128 | 115 | 0.90 |
| `BsCall` | 33.2 ns | 33.8 ns | 1.02 | 394 | 360 | 0.91 |
| `BsPut` | 39.4 ns | 41.3 ns | 1.05 | 396 | 357 | 0.90 |
| `BsCallDelta` | 22.7 ns | 17.8 ns | 0.79 | 198 | 180 | 0.91 |
| `CrrTreePriceEuropean/10` | 452 ns | 442 ns | 0.98 | 4,369 | 4,581 | 1.05 |
| `CrrTreePriceEuropean/100` | 6.03 µs | 7.35 µs | 1.22 | 57,649 | 59,320 | 1.03 |
| `CrrTreePriceEuropean/500` | 90.1 µs | 71.9 µs | 0.80 | 789,038 | 793,406 | 1.01 |
| `CrrTreePriceEuropean/2000` | 1.19 ms | 1.29 ms | 1.09 | 10,650,616 | 10,668,671 | 1.00 |
| `CrrTreePriceAmerican/10` | 376 ns | 447 ns | 1.19 | 4,774 | 5,188 | 1.09 |
| `CrrTreePriceAmerican/100` | 6.63 µs | 4.95 µs | 0.75 | 90,904 | 90,391 | 0.99 |
| `CrrTreePriceAmerican/500` | 102 µs | 98.8 µs | 0.96 | 1,605,293 | 1,498,877 | 0.93 |
| `CrrTreePriceAmerican/2000` | 1.46 ms | 1.47 ms | 1.01 | 23,665,621 | 21,740,642 | 0.92 |
| `PrintReport` | 1.35 ms | 1.36 ms | 1.01 | 15,475,242 | 14,918,884 | 0.96 |

## Observations

- **The full option trees (`OptionTree`, `EarlyExerciseNodes`) run
  0.77–0.97x the C++ instructions.** Both store the tree as a vector of
  rows and call the payoff through a type-erased function (`&dyn Fn` and
  `std::function`). Rust builds each row with `collect`, so it is allocated
  once at its final size. At N = 2000 the Rust time is 0.3–0.5x the C++
  time, a larger gap than the instruction counts explain. I did not isolate
  the cause; the trees hold 16 MB of rows at that size, so allocation and
  cache behaviour are the likely suspects.
- **`CrrTreePriceAmerican` is 0.92x the C++ instructions at N = 2000.** The
  first port ran 2.5x. Its loop indexed the row and the two power tables,
  `up[k]` and `down[n - k]`, with bounds checks, so LLVM did not vectorise it.
  GCC does vectorise it, because the in-place update
  `v[k] = f(v[k], v[k + 1])` has no loop-carried dependence. Now the update
  walks `Cell::as_slice_of_cells().windows(2)`, zipped with the two tables
  sliced to the row (`down` reversed), so the loop has no indices to check.
  A generic closure per payoff replaces the `is_call` branch, as GCC's
  unswitching does. The European loop has no exercise check and was already
  equal (1.00x).
- **The log-space binomial sums (`BinomialSum`, `EuropeanPriceSum`,
  `CrrCall`)** run 0.82–0.94x the instructions; the cost is the `exp` per
  term, which both languages call in glibc.
- **The Black-Scholes formulas** are 0.9x the instructions; `NormCdf` is the
  same polynomial in both.
- **`PrintReport`** is 0.96x the instructions and the same time: the
  report prices American trees up to N = 1000, which dominate it.
