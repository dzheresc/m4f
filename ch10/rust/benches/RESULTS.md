# Chapter 10: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch10/`.

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
| `Price/3` | 14.8 ns | 15.6 ns | 1.06 | 238 | 231 | 0.97 |
| `Price/10` | 60.3 ns | 47.3 ns | 0.78 | 770 | 721 | 0.94 |
| `Price/60` | 275 ns | 288 ns | 1.05 | 4,570 | 4,246 | 0.93 |
| `Price/360` | 1.65 µs | 1.72 µs | 1.04 | 27,370 | 25,396 | 0.93 |
| `Duration/3` | 18.1 ns | 15.4 ns | 0.85 | 264 | 250 | 0.95 |
| `Duration/10` | 57 ns | 51.3 ns | 0.90 | 852 | 785 | 0.92 |
| `Duration/60` | 235 ns | 299 ns | 1.27 | 5,052 | 4,610 | 0.91 |
| `Duration/360` | 1.65 µs | 1.69 µs | 1.03 | 30,252 | 27,379 | 0.91 |
| `LogReturn/3` | 36.5 ns | 37.7 ns | 1.03 | 532 | 544 | 1.02 |
| `LogReturn/10` | 128 ns | 111 ns | 0.87 | 1,603 | 1,577 | 0.98 |
| `LogReturn/60` | 636 ns | 648 ns | 1.02 | 9,666 | 9,028 | 0.93 |
| `LogReturn/360` | 3.98 µs | 3.03 µs | 0.76 | 57,820 | 53,878 | 0.93 |
| `Immunise` | 73.1 ns | 51.5 ns | 0.70 | 1,030 | 920 | 0.89 |
| `DynamicHedge/5` | 484 ns | 412 ns | 0.85 | 6,996 | 5,772 | 0.83 |
| `DynamicHedge/52` | 3.94 µs | 3.04 µs | 0.77 | 58,250 | 51,136 | 0.88 |
| `DynamicHedge/260` | 19.7 µs | 19 µs | 0.97 | 289,974 | 249,049 | 0.86 |
| `ForwardRates/3` | 76.1 ns | 44.4 ns | 0.58 | 1,032 | 556 | 0.54 |
| `ForwardRates/30` | 352 ns | 248 ns | 0.70 | 5,431 | 4,120 | 0.76 |
| `ForwardRates/360` | 3.24 µs | 2.2 µs | 0.68 | 52,641 | 47,888 | 0.91 |
| `PricesFromForwardRates/3` | 61.8 ns | 29.6 ns | 0.48 | 885 | 432 | 0.49 |
| `PricesFromForwardRates/30` | 259 ns | 163 ns | 0.63 | 3,633 | 2,538 | 0.70 |
| `PricesFromForwardRates/360` | 1.9 µs | 1.74 µs | 0.91 | 31,004 | 28,465 | 0.92 |
| `CouponBondPrice/3` | 5.99 ns | 7.15 ns | 1.19 | 93 | 95 | 1.02 |
| `CouponBondPrice/30` | 75.6 ns | 72.7 ns | 0.96 | 849 | 851 | 1.00 |
| `CouponBondPrice/360` | 775 ns | 748 ns | 0.97 | 10,089 | 10,091 | 1.00 |
| `YieldToMaturity/3` | 102 ns | 110 ns | 1.08 | 952 | 995 | 1.05 |
| `YieldToMaturity/30` | 792 ns | 759 ns | 0.96 | 12,347 | 11,658 | 0.94 |
| `YieldToMaturity/360` | 13.5 µs | 14 µs | 1.04 | 230,282 | 214,176 | 0.93 |
| `MoneyMarketAccount/3` | 46.3 ns | 21 ns | 0.45 | 694 | 229 | 0.33 |
| `MoneyMarketAccount/30` | 189 ns | 78.5 ns | 0.42 | 1,792 | 634 | 0.35 |
| `MoneyMarketAccount/360` | 2.14 µs | 1.55 µs | 0.73 | 8,734 | 5,792 | 0.66 |
| `PrintReport` | 38.1 µs | 44.3 µs | 1.16 | 500,339 | 673,060 | 1.35 |

## Observations

- **Bond pricing (`Price`, `Duration`, `LogReturn`, `Immunise`)** runs
  0.89–0.98x the C++ instructions. Each is one `exp` per cash flow, which
  both languages call in glibc; the loops walk the flows with iterators, so
  there are no bounds checks.
- **`DynamicHedge` is 0.83–0.88x** the instructions. Each rebalance
  re-prices both bonds with `price_and_duration` (one `exp` per flow) and
  solves the two-bond immunisation; I did not break down where the Rust
  saving comes from.
- **`ForwardRates`, `PricesFromForwardRates` and `MoneyMarketAccount` are
  0.33–0.92x.** The C++ `push_back`s into an empty vector; Rust reserves the
  final size first, so it allocates once. The gap is largest for short
  curves, where the reallocations are most of the work. A `reserve` in the
  C++ would close it (left for a separate PR).
- **`CouponBondPrice` was 1.14x in the first port.** The loop indexes
  `flow_amounts[i]` and `prices[j]`, which are checked against the lengths
  of `flow_times` and `maturities`, so every access kept a bounds check.
  Cutting both slices to their partners' lengths at the top of the function
  removed the checks and brought it to 1.00x. Its rows were re-run after the
  change.
- **`YieldToMaturity`** (Newton inside the bisection bracket) is 0.93–1.05x.
- **`PrintReport`** is 1.35x the instructions and 1.16x the time: Rust's
  `fmt` costs more than `std::format` per number, as in the other chapters.
