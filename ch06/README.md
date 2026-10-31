# Chapter 6 — Forward and Futures Contracts

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 6. There are four
parallel implementations with the same API, and all four print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch06/
  cpp/      C++20, CMake        include/m4f/{forwards,futures}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch06/{Forwards,Futures,Report}.kt, src/test/
  python/   Python 3.10+        forwards.py, futures.py, ch06.py, ch06_test.py
  rust/     Rust 2024, Cargo    src/{forwards,futures,report}.rs, tests/, benches/
  run_all.sh                    build + test + run all four and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint, rustfmt + clippy (whichever are installed)
  bench.sh                      C++ microbenchmarks (Google Benchmark), --callgrind adds instruction counts;
                                --rust (Criterion), --rust-callgrind, --compare (Rust vs C++ table)
```

Forwards use continuous compounding, as the chapter does. Futures pricing uses the binomial tree with `r` per step.

| Section | Formula | API (C++ / Kotlin; Python and Rust use snake_case) |
|---|---|---|
| 6.1.1 Forward price | `F(0,T) = S(0)e^(rT)`; with dividends `(S(0) - PV(div))e^(rT)`; with a dividend yield or foreign rate `S(0)e^((r - r_div)T)`; arbitrage profit `\|F_quoted - S(0)e^(rT)\|` | `forwardPrice`, `forwardPriceDividends`, `forwardPriceYield`, `forwardArbitrageProfit` |
| 6.1.2 Value of a forward | `V(t) = (F(t,T) - F(0,T))e^(-r(T-t))` | `longForwardValue` |
| 6.2.1 Futures pricing | `f(n) = E*(S(N) \| S(n))`, by backward induction; equals the forward price when `r` is constant | `futuresTree` |
| 6.2.1 Marking to market | Daily settlement `size·Δf`; a margin call restores the initial margin when the balance falls below the maintenance level | `marginAccount` |
| 6.2.2 Hedging with futures | Minimum variance hedge ratio `h* = Cov(ΔS, Δf)/Var(Δf)` | `minVarianceHedgeRatio`, `hedgedStd` |

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch06
cd kotlin && ./gradlew test run
cd python && python3 -m unittest ch06_test && python3 ch06.py
cd rust && cargo test --release && cargo run --release   # Rust 1.86+
```

## Code style

The code follows the Google style guides for C++ and Python and the Android Kotlin style guide;
Rust, which has no Google guide, is rustfmt- and clippy-clean;
see [STYLE.md](../STYLE.md). API names in this README are the Kotlin ones: C++ uses PascalCase
for functions (`riskNeutralProb` is `RiskNeutralProb`) and Python and Rust use snake_case. Run
`./check_style.sh` to check, or `./check_style.sh --fix` to reformat.

## Benchmarks

`cpp/benchmarks/` holds Google Benchmark microbenchmarks for the C++ code. They are
off in a normal build; `./bench.sh` configures a separate Release build in
`cpp/build-bench` with `-DM4F_BUILD_BENCHMARKS=ON` (using an installed Google Benchmark,
or downloading v1.9.1) and runs them five times. `./bench.sh --callgrind` also runs
`cpp/benchmarks/callgrind_ir.py`, which reports exact instruction counts per iteration
under Valgrind; use those to compare versions on a noisy machine. Baseline numbers and
notes are in [`cpp/benchmarks/RESULTS.md`](cpp/benchmarks/RESULTS.md).

`rust/benches/` has the same cases for Rust: `cases.rs` lists them once, and both the
Criterion harness (`./bench.sh --rust`) and a callgrind runner (`examples/bench_ir.rs`,
`./bench.sh --rust-callgrind`) use that list. `./bench.sh --compare` runs both languages and
prints a Rust vs C++ table of times and instruction counts; the results are in
[`rust/benches/RESULTS.md`](rust/benches/RESULTS.md).
