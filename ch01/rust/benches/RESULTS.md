# Chapter 1: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch01/`.

**Machine:** cloud VM, 4 vCPU Intel Xeon at 2.1 GHz (shared). C++: GCC 13.3,
`-O3` Release, Google Benchmark 1.9.1, median of 5 repetitions. Rust: rustc
1.97, release profile (`opt-level = 3`, one codegen unit, fat LTO), Criterion
0.8, median of 100 samples. Instruction counts (Ir per iteration) come from
Valgrind 3.22 callgrind for both, as (Ir(N2) − Ir(N1)) / (N2 − N1), so set-up
cancels. Ratios below 1 mean Rust is faster or runs fewer instructions.

Wall-clock times on this VM vary by up to 10% run to run; the Ir counts are
exact. The random inputs are drawn from the same ranges in both languages but
by different generators (`std::mt19937_64` in C++, SplitMix64 in Rust), so
data-dependent cases such as `BestCycleScaling` see different markets.

| Benchmark | C++ time | Rust time | Rust/C++ | C++ Ir | Rust Ir | Rust/C++ |
|---|---:|---:|---:|---:|---:|---:|
| `Construct` | 2.8 ns | 1.51 ns | 0.54 | 31 | 34 | 1.10 |
| `IsArbitrageFree` | 3.67 ns | 2.59 ns | 0.71 | 22 | 24 | 1.11 |
| `Arbitrage` | 3.8 ns | 2.6 ns | 0.68 | 23 | 26 | 1.16 |
| `RiskNeutralProb` | 4.89 ns | 3.74 ns | 0.77 | 22 | 22 | 1.00 |
| `Replicate` | 2.16 ns | 2.49 ns | 1.15 | 27 | 27 | 1.01 |
| `PriceByReplication` | 2.61 ns | 2.51 ns | 0.96 | 29 | 27 | 0.94 |
| `RiskNeutralPrice` | 6.08 ns | 4.99 ns | 0.82 | 39 | 38 | 0.98 |
| `PortfolioRisk` | 6.52 ns | 3.78 ns | 0.58 | 44 | 43 | 0.98 |
| `HedgedPurchaseCost` | 4.02 ns | 3.82 ns | 0.95 | 39 | 39 | 0.99 |
| `ForwardArbitrage` | 2.17 ns | 2.74 ns | 1.26 | 22 | 26 | 1.21 |
| `FxMarketConstruct` | 776 ns | 535 ns | 0.69 | 10,096 | 11,028 | 1.09 |
| `BestLeg` | 35 ns | 20.7 ns | 0.59 | 493 | 310 | 0.63 |
| `BestCycleBook` | 233 ns | 109 ns | 0.47 | 3,315 | 1,778 | 0.54 |
| `BestCycleScaling/3` | 254 ns | 99.7 ns | 0.39 | 3,495 | 1,742 | 0.50 |
| `BestCycleScaling/4` | 396 ns | 215 ns | 0.54 | 5,319 | 3,704 | 0.70 |
| `BestCycleScaling/5` | 801 ns | 834 ns | 1.04 | 14,824 | 14,299 | 0.96 |
| `BestCycleScaling/6` | 4.03 µs | 4.08 µs | 1.01 | 68,226 | 71,388 | 1.05 |
| `BestCycleScaling/7` | 26.6 µs | 27 µs | 1.01 | 431,038 | 463,966 | 1.08 |
| `BestCycleScaling/8` | 192 µs | 209 µs | 1.09 | 3,292,582 | 3,550,464 | 1.08 |
| `PrintReport` | 27.8 µs | 35.5 µs | 1.28 | 349,908 | 425,944 | 1.22 |

## Observations

- **One-step market (`Construct` to `ForwardArbitrage`).** Both compile to
  the same 22–44 instructions; the Rust counts are within ±20%. A few extra
  instructions in Rust are the `assert!` branches and the `Option` tag. The
  times are 2–7 ns in both and differ mostly by noise.
- **Inlining across crates matters.** Without LTO, `Construct` took 8 ns and
  48 Ir in Rust: `OneStepMarket::new` lives in the library crate and the
  benchmark is a separate crate, so rustc did not inline it, while the C++
  constructor is in a header. Fat LTO (or `#[inline]`) gives Rust the same
  whole-program view, and the profile in `Cargo.toml` turns it on.
- **`BestLeg` and `BestCycleBook` are about 2x faster in Rust** (0.59x and
  0.47x the time). `best_leg` returns `Option<&FxLeg>` and `FxCycle` holds
  `&FxLeg`s borrowed from the market, so nothing is copied; the C++ versions
  return `FxLeg` by value, which copies three `std::string`s per leg. The
  borrow checker makes the zero-copy API safe to hand out.
- **`BestCycleScaling/5..8`** is the depth-first search itself, with no copies
  on the hot path in either language. Rust is within 1–9% of C++ in time and
  runs 0–8% more instructions, mostly bounds checks on the `visited` and
  best-leg tables.
- **`PrintReport` is 1.2–1.3x slower in Rust.** The report is about 70 lines
  of `{:.4}` formatting; Rust's float formatter (Grisu with a Dragon4
  fallback for exact digits) runs more instructions than libstdc++'s
  `std::format` (Ryu printf). Neither is on a hot path.
