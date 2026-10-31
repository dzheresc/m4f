# Chapter 2: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch02/`.

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
| `SimpleValue` | 0.812 ns | 0.776 ns | 0.96 | 12 | 11 | 0.92 |
| `SimpleRate` | 2.44 ns | 2.13 ns | 0.87 | 16 | 14 | 0.88 |
| `PeriodicValue` | 17.6 ns | 12.7 ns | 0.73 | 150 | 145 | 0.97 |
| `EffectiveRate` | 18.8 ns | 16.2 ns | 0.86 | 147 | 144 | 0.98 |
| `AnnuityPresentValue` | 16.5 ns | 15.8 ns | 0.96 | 158 | 151 | 0.96 |
| `AnnuityFutureValue` | 16.8 ns | 15.7 ns | 0.94 | 154 | 147 | 0.95 |
| `LoanInstallment` | 17.5 ns | 16.1 ns | 0.92 | 158 | 154 | 0.97 |
| `ContinuousValue` | 5.06 ns | 4.87 ns | 0.96 | 74 | 69 | 0.93 |
| `ContinuousFromPeriodic` | 6.72 ns | 4.46 ns | 0.66 | 70 | 66 | 0.94 |
| `PeriodicFromContinuous` | 6.28 ns | 5.49 ns | 0.87 | 77 | 73 | 0.95 |
| `PeriodicFromPeriodic` | 17 ns | 16.9 ns | 1.00 | 149 | 147 | 0.99 |
| `ZeroCouponPricePeriodic` | 13.4 ns | 13 ns | 0.97 | 143 | 140 | 0.98 |
| `ZeroCouponPriceContinuous` | 4.9 ns | 4.62 ns | 0.94 | 70 | 66 | 0.94 |
| `ImpliedRatePeriodic` | 13.2 ns | 12.8 ns | 0.98 | 144 | 141 | 0.98 |
| `CouponBondPrice/3` | 21.6 ns | 15.3 ns | 0.71 | 189 | 175 | 0.92 |
| `CouponBondPrice/10` | 24.1 ns | 20.9 ns | 0.87 | 204 | 189 | 0.92 |
| `CouponBondPrice/30` | 28.4 ns | 27.2 ns | 0.96 | 218 | 203 | 0.93 |
| `CouponBondPrice/60` | 28.9 ns | 26.3 ns | 0.91 | 221 | 207 | 0.94 |
| `CouponBondPrice/360` | 25.7 ns | 27 ns | 1.05 | 206 | 195 | 0.94 |
| `YieldToMaturity/3` | 195 ns | 195 ns | 1.00 | 1,001 | 1,021 | 1.02 |
| `YieldToMaturity/10` | 236 ns | 246 ns | 1.04 | 1,418 | 1,434 | 1.01 |
| `YieldToMaturity/30` | 386 ns | 336 ns | 0.87 | 2,311 | 2,270 | 0.98 |
| `YieldToMaturity/60` | 473 ns | 490 ns | 1.04 | 3,339 | 3,193 | 0.96 |
| `YieldToMaturity/360` | 1.7 µs | 1.66 µs | 0.98 | 11,945 | 11,031 | 0.92 |
| `MoneyMarketRollover/4` | 4.6 ns | 5.15 ns | 1.12 | 24 | 38 | 1.58 |
| `MoneyMarketRollover/120` | 198 ns | 164 ns | 0.83 | 314 | 186 | 0.59 |
| `MoneyMarketRollover/1024` | 3.88 µs | 3.9 µs | 1.00 | 2,574 | 1,429 | 0.56 |
| `PrintReport` | 8.24 µs | 11.9 µs | 1.44 | 104,500 | 139,841 | 1.34 |

## Observations

- **Interest formulas (2.1) and zero-coupon bonds (2.2.1)** are one `pow`,
  `exp` or `log` call into the same glibc libm from both languages, plus a few
  arithmetic operations. Rust runs 1–12% fewer instructions and is up to 1.5x
  faster on the cheapest ones. Nearly all of the instructions are inside
  libm, so the two languages differ only in the few around the call.
- **`CouponBondPrice`** (closed-form annuity with `ln_1p` and `exp_m1`) is
  about 7% fewer instructions in Rust at every length; it is O(1) in both.
- **`YieldToMaturity` exposed a Rust pitfall.** The first port wrote the
  slope loop as `for k in 1..=n`. An inclusive range carries an extra
  "exhausted" flag that LLVM does not remove, and at n = 360 that cost
  20,209 Ir against C++'s 11,945 (1.69x). Rewriting it as `1..n + 1` brought
  it to 11,031 Ir, 8% below C++. The table shows the fixed code. The other
  chapters' hot loops use half-open ranges for the same reason.
- **`MoneyMarketRollover`** is a chain of divisions, so its speed is the
  divider's latency in both languages (3.9 µs at 1024 steps). Rust's `fold`
  over a slice executes 44% fewer instructions at 1024 steps, but the time is
  the same because the divisions dominate.
- **`PrintReport` is about 1.4x slower in Rust**: float formatting, as in
  chapter 1.
