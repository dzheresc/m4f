# Chapter 9 — Financial Engineering

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 9. There are four
parallel implementations with the same API, and all four print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch09/
  cpp/      C++20, CMake        include/m4f/{black_scholes,hedging,risk}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch09/{BlackScholes,Hedging,Risk,Report}.kt, src/test/
  python/   Python 3.10+        black_scholes.py, hedging.py, risk.py, ch09.py, ch09_test.py
  rust/     Rust 2024, Cargo    src/{black_scholes,hedging,risk,report}.rs, tests/, benches/
  run_all.sh                    build + test + run all four and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint, rustfmt + clippy (whichever are installed)
  bench.sh                      C++ microbenchmarks (Google Benchmark), --callgrind adds instruction counts;
                                --rust (Criterion), --rust-callgrind, --compare (Rust vs C++ table)
```

Options are European, priced by Black-Scholes with a continuously compounded `r`. Sections 9.2–9.3 model the
stock as lognormal, `S(T) = S(0) exp(mT + σ√T Z)`, where `m` is the expected log return (the real-world drift,
distinct from `r`).

| Section | Formula | API (C++ / Kotlin; Python and Rust use snake_case) |
|---|---|---|
| 9.1.1 Delta hedging | Written options + `Δ` shares + money market, value 0 at t=0; value after a jump in `S`, with and without the passage of time; weekly rebalancing along a path | `deltaHedge`, `portfolioValue`, `rebalancePath` |
| 9.1.2 Greek parameters | `Δ = N(d1)`, `Γ = n(d1)/(Sσ√T)`, `Θ`, `vega = S n(d1)√T`, `ρ` for calls and puts; position Greeks are quantity-weighted sums | `bsGreeks`, `totalGreek` |
| 9.1.2 Delta-gamma, delta-vega | Second option quantity `y = -Γ_pos/Γ_2` (or vega), then shares to zero the delta | `deltaGreekHedge(..., Greek::Gamma / Vega)` |
| 9.1.3 Applications | Combined move in `S`, `σ` and time; theta decay per week | `portfolioValue` |
| 9.2.1 Value at Risk | `VaR = V(0) - V_q(T)` with `V_q` the `(1-conf)` quantile; `S_q = S(0) exp(mT + σ√T N⁻¹(1-conf))`; put-hedged holdings; `E max(S(T)-X,0)` in closed form | `Lognormal`, `valueAtRisk`, `putHedgedHolding`, `expectedReturn` |
| 9.2.2 Case study | Cheapest put strike bringing VaR under a target | report only |
| 9.3.1 Speculating | All-in stock vs calls vs mixtures: terminal values, expected return, breakeven, `P(loss)`, VaR | `allInStock`, `allInCalls`, `mix`, `breakeven`, `probLoss` |
| 9.3.2 Case study | Bull spread vs calls vs stock under a bullish view | `bullSpread` |

`normInv` uses Acklam's rational approximation plus one Halley step, and `breakeven` uses fixed-count bisection, so the four languages agree to the last printed digit.

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch09
cd kotlin && ./gradlew test run
cd python && python3 -m unittest ch09_test && python3 ch09.py
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
