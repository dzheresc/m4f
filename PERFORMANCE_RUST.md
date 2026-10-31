# Rust vs C++ performance report

Every chapter now has a Rust port of the optimised C++ code from
[PERFORMANCE.md](PERFORMANCE.md), with the same algorithms, the same
benchmark cases and byte-identical output. This report compares the two.

**Summary:** across 317 benchmarks (the report printers left out), Rust takes
0.85x the C++ time and runs 0.91x the C++ instructions (geometric means).
177 benchmarks are within ±10% of the C++ instruction count, 93 run fewer
than 0.9x, and 47 run more than 1.1x. On the chapters' headline benchmarks
Rust runs 0.85–1.08x the C++ instructions, except chapter 6's futures tree
at 0.62x. The report printers are the one place
Rust is consistently slower, at 1.2–1.4x the instructions.

The C++ was not changed for this comparison. Where Rust is much faster, the
reason is usually a C++ `push_back` without `reserve`, which is noted
below and left for a separate change.

## Method

- **Metrics:** time per call and instructions per call (Ir). Ir comes from
  Valgrind callgrind for both languages, as (Ir(N2) − Ir(N1)) / (N2 − N1),
  so set-up cancels. Ir is exact and repeatable; wall-clock times on the
  shared VM vary by up to 10% between runs. Ratios below 1 mean Rust is
  faster or runs fewer instructions.
- **C++:** GCC 13.3, `-O3` Release, Google Benchmark 1.9.1, median of 5
  repetitions (as in PERFORMANCE.md).
- **Rust:** rustc 1.97, edition 2024, release profile with `opt-level = 3`,
  one codegen unit and fat LTO; Criterion 0.8, median of 100 samples.
- **Same cases:** each chapter's `rust/benches/cases.rs` mirrors
  `cpp/benchmarks/*_benchmark.cc`, and a callgrind runner
  (`rust/examples/bench_ir.rs`) uses the same list. Random inputs come from
  the same ranges but different generators (`std::mt19937_64` and
  SplitMix64).
- **Machine:** cloud VM, 4 vCPU Intel Xeon at 2.1 GHz (shared), Valgrind
  3.22.
- **Run it:** `./bench.sh --compare [regex]` in a chapter prints the table.
  Each chapter's full table and notes are in `chNN/rust/benches/RESULTS.md`.

## Headline per chapter

The benchmarks from PERFORMANCE.md's headline table.

| Ch | Benchmark | C++ time | Rust time | Time ratio | C++ Ir | Rust Ir | Ir ratio | PR |
|---:|---|---:|---:|---:|---:|---:|---:|---|
| 1 | BestCycleScaling/8 | 192 µs | 209 µs | 1.09 | 3,292,582 | 3,550,464 | 1.08 | #50 |
| 2 | CouponBondPrice/360 | 25.7 ns | 27 ns | 1.05 | 206 | 195 | 0.94 | #51 |
| 2 | YieldToMaturity/360 | 1.7 µs | 1.66 µs | 0.98 | 11,945 | 11,031 | 0.92 | #51 |
| 3 | ExpectedPrice/1000 | 16.3 µs | 15.1 µs | 0.93 | 209,769 | 208,902 | 1.00 | #52 |
| 4 | ExpectedDiscountedValue/16 | 2.27 ms | 3.04 ms | 1.34 | 16,021,212 | 15,617,462 | 0.97 | #53 |
| 5 | MinVariancePortfolio/200 | 656 µs | 610 µs | 0.93 | 6,977,031 | 6,658,133 | 0.95 | #54 |
| 6 | FuturesTree/1000 | 1.05 ms | 399 µs | 0.38 | 13,564,115 | 8,412,516 | 0.62 | #55 |
| 7 | CrrAmericanPut/2000 | 1.29 ms | 1.35 ms | 1.04 | 19,723,093 | 16,724,449 | 0.85 | #56 |
| 8 | CrrTreePriceAmerican/2000 | 1.46 ms | 1.47 ms | 1.01 | 23,665,621 | 21,740,642 | 0.92 | #57 |
| 9 | NormInv | 28.6 ns | 30 ns | 1.05 | 253 | 226 | 0.89 | #58 |
| 10 | YieldToMaturity/3 | 102 ns | 110 ns | 1.08 | 952 | 995 | 1.05 | #59 |
| 11 | BondPrices/200 | 129 µs | 120 µs | 0.93 | 1,752,392 | 1,796,522 | 1.03 | #60 |

## All benchmarks per chapter

Geometric means of the Rust/C++ ratios over each chapter's benchmarks,
without `PrintReport`, and the report printer on its own.

| Ch | Benchmarks | Time | Ir | Ir > 1.1x | Ir < 0.9x | PrintReport time | PrintReport Ir |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 19 | 0.76 | 0.93 | 3 | 4 | 1.28 | 1.22 |
| 2 | 27 | 0.92 | 0.94 | 1 | 3 | 1.44 | 1.34 |
| 3 | 31 | 0.92 | 0.85 | 1 | 8 | 0.84 | 0.97 |
| 4 | 28 | 0.97 | 1.02 | 14 | 6 | 1.61 | 1.20 |
| 5 | 44 | 0.93 | 1.05 | 17 | 10 | 1.21 | 1.36 |
| 6 | 18 | 0.62 | 0.66 | 0 | 10 | 1.21 | 1.23 |
| 7 | 22 | 0.94 | 0.98 | 5 | 7 | 1.27 | 0.98 |
| 8 | 34 | 0.75 | 0.90 | 0 | 15 | 1.01 | 0.96 |
| 9 | 28 | 0.93 | 0.99 | 6 | 4 | 1.29 | 1.25 |
| 10 | 31 | 0.83 | 0.82 | 0 | 11 | 1.16 | 1.35 |
| 11 | 35 | 0.78 | 0.86 | 0 | 15 | 1.39 | 1.19 |

The report printers of chapters 3, 7 and 8 spend their time pricing trees,
not formatting, so they follow the numeric code.

## Where Rust is faster

- **Vectors allocated once.** `collect` from an iterator of known length
  (`windows`, `map` over a slice or a range) allocates the result at its
  final size and lets LLVM vectorise the loop that fills it. The C++ often
  `push_back`s into an empty vector with no `reserve`. This accounts for
  chapter 3's `Returns` (0.23x Ir), chapter 4's `OneStepValues` (0.15x),
  chapter 6's `FuturesTree`, `MarginAccount` and `HedgedStd` (0.35–0.62x),
  chapter 10's forward-rate and money-market curves (0.33–0.92x) and
  chapter 11's `RateLattice` (0.52x, where the C++ `resize` zero-fills each
  row before writing it). A `reserve` in the C++ would close most of these
  gaps.
- **Borrowing instead of copying.** Chapter 1's `best_leg` returns
  `Option<&FxLeg>` and the cycles hold references into the market, where the
  C++ returns `FxLeg` by value and copies three `std::string`s per leg
  (0.47–0.59x the time).
- **Big trees.** Chapter 8's option trees at N = 2000 take 0.3–0.5x the C++
  time but 0.77–0.97x the instructions. The cause was not isolated; the trees
  hold 16 MB of rows, so allocation and cache behaviour are the likely
  suspects.

## Where Rust is slower

- **Formatting.** Every chapter's `PrintReport` that is mostly text runs
  1.2–1.4x the instructions of the C++ `std::format`. Rust's float
  formatter does more work per number; none of it is on a hot path.
- **Ordered floating-point sums.** LLVM will not vectorise a floating-point
  sum without permission to reorder it. GCC vectorises the per-step terms of
  chapter 4's `Gains` and `DiscountedGains` with SSE2 and then adds them in
  order, so Rust runs 1.3–1.5x the instructions at about the same speed.
- **`Vec<Vec<f64>>` lookups.** `x[j][n]` keeps a bounds check per access
  (chapter 4's `SelfFinancingMulti` at 1.5x Ir, the same time). The checks
  are well predicted, so they cost instructions rather than time.
- **Inlining.** In chapter 9 GCC inlines `BsPrice` and `ExpectedCallPayoff`
  into the strategy functions and shares `exp` calls between legs; LLVM
  does not, so `ValueAtRisk` and its neighbours run 1.2–1.7x the
  instructions.
- **Small inputs.** At two or three elements some functions are 20–50
  instructions, and Rust spends a fixed few more on saving registers and
  returning `Option<Vec<f64>>` through memory (chapter 4's scenario scans).

## Lessons for writing fast Rust

These came out of first ports that ran 1.5–3.6x the C++ instructions and were
fixed. The rules are in [STYLE.md](STYLE.md#rust).

1. **Index only slices that were cut to the loop's length.** Indexing a
   longer `Vec` by a computed index keeps a bounds check on every access, and
   the check also stops LLVM from vectorising the loop. Iterating with `zip`
   and `windows`, or slicing once before the loop
   (`let row = &mut values[..n + 2]`), removes both problems. This took
   chapter 5's Cholesky solve from 2.5x to 0.95x, chapter 4's gains from
   2.7x to 1.5x, chapter 6's hedge ratio from 1.46x to 1.02x and chapter 10's
   coupon bond price from 1.14x to 1.00x.
2. **In-place stencils need `Cell`.** The tree step `v[k] = f(v[k], v[k + 1])`
   cannot borrow `v` twice. Walking `Cell::from_mut(row).as_slice_of_cells()`
   with `windows(2)`, zipped with the price tables, gives LLVM a loop with no
   indices, and it vectorises like GCC's. Chapter 7's American tree went
   from 2.4x to 0.85x and chapter 8's from 2.5x to 0.92x.
3. **`step_by(2)` blocks vectorisation.** Chapter 7's price index
   `N − n + 2k` read every other table entry. Splitting the table by parity
   made each row one contiguous slice.
4. **Use half-open ranges in hot loops.** `1..=n` keeps an extra flag that
   LLVM does not remove; chapter 2's yield solver ran 1.69x until it became
   `1..n + 1`.
5. **Use generics where GCC would unswitch.** A generic closure per payoff
   gives each payoff its own copy of the loop, as GCC does by unswitching on
   `is_call`.
6. **Inline across crates.** Without LTO the benchmark crate could not inline
   the library's small functions (chapter 1's constructor took 48 Ir; with LTO it
   takes 34, against 31 in C++). The release profile turns on fat LTO.
7. **`#[inline(always)]` where inlining removes work.** Chapter 9's
   `total_greek` asks for one Greek; once `bs_greeks` is inlined, the
   constant lets LLVM drop the other four. That took it from 3.6x to 0.92x.
   Use it only in such cases.

## Not compared

- Kotlin and Python are covered by [PERFORMANCE_KOTLIN.md](PERFORMANCE_KOTLIN.md)
  and [PERFORMANCE_PYTHON.md](PERFORMANCE_PYTHON.md).
- Multithreading and SIMD intrinsics: neither language uses them here.
