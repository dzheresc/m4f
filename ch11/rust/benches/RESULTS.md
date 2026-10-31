# Chapter 11: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch11/`.

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
| `RateLattice/10` | 225 ns | 137 ns | 0.61 | 3,623 | 2,601 | 0.72 |
| `RateLattice/100` | 5.23 µs | 4.15 µs | 0.79 | 61,064 | 51,928 | 0.85 |
| `RateLattice/1000` | 1.76 ms | 1.58 ms | 0.90 | 7,152,194 | 3,691,035 | 0.52 |
| `BondLattice/10` | 608 ns | 460 ns | 0.76 | 8,343 | 7,208 | 0.86 |
| `BondLattice/100` | 34.9 µs | 21.9 µs | 0.63 | 526,578 | 458,604 | 0.87 |
| `BondLattice/1000` | 3.53 ms | 3 ms | 0.85 | 50,756,669 | 44,228,262 | 0.87 |
| `ImpliedProbs/10` | 761 ns | 377 ns | 0.50 | 10,853 | 6,130 | 0.56 |
| `ImpliedProbs/100` | 42.2 µs | 36.3 µs | 0.86 | 614,038 | 483,667 | 0.79 |
| `ImpliedProbs/1000` | 3.79 ms | 2.38 ms | 0.63 | 48,548,024 | 47,646,058 | 0.98 |
| `BondPrices/10` | 580 ns | 517 ns | 0.89 | 7,360 | 7,131 | 0.97 |
| `BondPrices/50` | 9.76 µs | 9.42 µs | 0.96 | 118,654 | 118,750 | 1.00 |
| `BondPrices/200` | 129 µs | 120 µs | 0.93 | 1,752,392 | 1,796,522 | 1.03 |
| `MoneyMarket/10` | 145 ns | 71.8 ns | 0.50 | 1,885 | 1,106 | 0.59 |
| `MoneyMarket/100` | 758 ns | 434 ns | 0.57 | 9,892 | 9,044 | 0.91 |
| `MoneyMarket/1000` | 6.87 µs | 6.09 µs | 0.89 | 85,250 | 89,338 | 1.05 |
| `BondOptionEuropean/10` | 874 ns | 604 ns | 0.69 | 12,389 | 9,992 | 0.81 |
| `BondOptionEuropean/100` | 49 µs | 35.8 µs | 0.73 | 667,666 | 579,981 | 0.87 |
| `BondOptionEuropean/1000` | 4.52 ms | 3.01 ms | 0.66 | 63,612,270 | 55,438,146 | 0.87 |
| `BondOptionAmerican/10` | 745 ns | 652 ns | 0.88 | 13,035 | 10,577 | 0.81 |
| `BondOptionAmerican/100` | 48.1 µs | 38.5 µs | 0.80 | 705,046 | 628,834 | 0.89 |
| `BondOptionAmerican/1000` | 4.56 ms | 4.13 ms | 0.90 | 67,305,106 | 60,318,512 | 0.90 |
| `PayerSwap/10` | 922 ns | 818 ns | 0.89 | 13,497 | 12,569 | 0.93 |
| `PayerSwap/100` | 70.7 µs | 47.7 µs | 0.67 | 980,305 | 937,908 | 0.96 |
| `PayerSwap/1000` | 6.78 ms | 4.88 ms | 0.72 | 95,796,956 | 91,772,726 | 0.96 |
| `Cap/10` | 921 ns | 838 ns | 0.91 | 13,682 | 12,624 | 0.92 |
| `Cap/100` | 74.9 µs | 64.4 µs | 0.86 | 997,905 | 942,958 | 0.94 |
| `Cap/1000` | 7.79 ms | 8.74 ms | 1.12 | 97,547,920 | 92,273,226 | 0.95 |
| `Floor/10` | 999 ns | 688 ns | 0.69 | 13,972 | 12,734 | 0.91 |
| `Floor/100` | 69.2 µs | 67 µs | 0.97 | 1,023,305 | 953,058 | 0.93 |
| `Floor/1000` | 7.56 ms | 6.59 ms | 0.87 | 100,051,920 | 93,274,226 | 0.93 |
| `Caplet/10` | 422 ns | 343 ns | 0.81 | 5,340 | 5,175 | 0.97 |
| `Caplet/100` | 25.4 µs | 25.7 µs | 1.01 | 401,574 | 369,843 | 0.92 |
| `Caplet/1000` | 2.57 ms | 2.52 ms | 0.98 | 39,119,175 | 35,645,549 | 0.91 |
| `SwapRate/10` | 3.33 ns | 2.31 ns | 0.69 | 53 | 52 | 0.98 |
| `SwapRate/100` | 34.8 ns | 23 ns | 0.66 | 278 | 181 | 0.65 |
| `PrintReport` | 28.9 µs | 40.1 µs | 1.39 | 404,737 | 481,250 | 1.19 |

## Observations

- **`RateLattice` is 0.52–0.85x the C++ instructions.** The C++ `resize`s
  each row, which zero-fills it, then writes every node; Rust `collect`s
  each row from a range, so the row is written once and the loop is
  vectorised. At 1000 steps that halves the instruction count, although the
  time (0.9x) is bound by the 4 MB of rows being allocated and written.
- **The backward inductions (`BondLattice`, `BondOption*`, `PayerSwap`,
  `Cap`, `Floor`, `Caplet`)** run 0.81–0.97x the instructions. All go
  through `discount_back`, which calls the payment and early-exercise
  functions through `&dyn Fn` like the C++ `std::function`, and one `exp`
  per node in both languages. Each row is sliced to its length before the
  inner loop, so there are no bounds checks.
- **`BondPrices`** (forward induction over the state prices) is 0.97–1.03x.
  The first port computed the forward step twice; that was a porting bug
  with the same output, fixed before these numbers were taken.
- **`ImpliedProbs`** is 0.56x at 10 steps and 0.98x at 1000. Each node is
  one `exp` and a division in both languages, which dominate large trees;
  small trees gain from building each row at its final size.
- **`MoneyMarket`** is 0.59x at 10 steps (the C++ `push_back`s without
  `reserve`) and 1.05x at 1000, where the `Vec<Vec>` row lookups
  `rates[n][k]` keep their bounds checks.
- **`SwapRate`** is one sum over the bond prices; at 100 prices Rust runs
  0.65x the instructions because LLVM unrolls the loop further. The sum
  stays in order in both.
- **`PrintReport`** is 1.19x the instructions and 1.4x the time: Rust's
  `fmt` costs more than `std::format` per number, as in the other chapters.
