# Chapter 7 — Options: General Properties

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 7. There are four
parallel implementations with the same API, and all four print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch07/
  cpp/      C++20, CMake        include/m4f/{options,binomial}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch07/{Options,Binomial,Report}.kt, src/test/
  python/   Python 3.10+        options.py, binomial.py, ch07.py, ch07_test.py
  rust/     Rust 2024, Cargo    src/{options,binomial,report}.rs, tests/, benches/
  run_all.sh                    build + test + run all four and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint, rustfmt + clippy (whichever are installed)
  bench.sh                      C++ microbenchmarks (Google Benchmark), --callgrind adds instruction counts;
                                --rust (Criterion), --rust-callgrind, --compare (Rust vs C++ table)
```

Interest is compounded continuously. The model prices used in 7.3–7.5 come from a 200-step Cox-Ross-Rubinstein
tree (`crrPrice`; σ = 0.2, r = 0.05), which Chapter 8 develops properly.

| Section | Formula | API (C++ / Kotlin; Python and Rust use snake_case) |
|---|---|---|
| 7.1 Definitions | Payoffs `(S - X)⁺`, `(X - S)⁺`; profit = payoff − premium·e^(rT) | `callPayoff`, `putPayoff`, `profit` |
| 7.2 Put-call parity | `C - P = S(0) - div - Xe^(-rT)`; a gap locks in `\|gap\|e^(rT)` | `parityGap` |
| 7.3.1 European bounds | `max(0, S(0) - div - Xe^(-rT)) ≤ C ≤ S(0) - div`, `max(0, Xe^(-rT) - S(0) + div) ≤ P ≤ Xe^(-rT)` | `europeanCallBounds`, `europeanPutBounds`, `boundViolation` |
| 7.3.2 American calls, no dividends | `C_A = C_E` (early exercise never pays) | `crrPrice(..., american=true)` |
| 7.3.3 American options | `S(0) - X ≤ C_A - P_A ≤ S(0) - Xe^(-rT)` | `americanParityBounds` |
| 7.4.1 Dependence on X and S | Decreasing and convex in `X`, `C(X1) - C(X2) ≤ e^(-rT)(X2 - X1)`; butterfly arbitrage when convexity fails | `butterflyGap` |
| 7.4.2 American options | Prices increase with `T` | `crrPrice` |
| 7.5 Time value | price − intrinsic value; can be negative for a deep in-the-money European put | `crrPrice`, payoffs |

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch07
cd kotlin && ./gradlew test run
cd python && python3 -m unittest ch07_test && python3 ch07.py
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
