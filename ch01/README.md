# Chapter 1 — Introduction: A Simple Market Model

Code companion for *Mathematics for Finance: An Introduction to Financial Engineering*
(Capiński & Zastawniak), Chapter 1, in four parallel implementations with the same API
and byte-identical output (`expected_output.txt`).

```
ch01/
  cpp/      C++20, CMake      include/m4f/{one_step,forward,fx_arbitrage,check}.h, src/{fx_arbitrage,report}.cc, tests/
  kotlin/   Kotlin 2.1, Gradle src/main/kotlin/m4f/ch01/OneStepMarket.kt, Forward.kt, FxArbitrage.kt, Report.kt, src/test/
  python/   Python 3.10+, stdlib only  one_step.py, forward.py, fx_arbitrage.py, ch01.py, one_step_test.py
  rust/     Rust 2024, Cargo   src/{one_step,forward,fx_arbitrage,report}.rs, tests/ch01_test.rs, benches/
  run_all.sh            build + test + run all four and diff against expected_output.txt
  check_style.sh        clang-format, cpplint, pyink, pylint, ktlint, rustfmt + clippy (whichever are installed)
  bench.sh              C++ microbenchmarks (Google Benchmark), --callgrind adds instruction counts;
                        --rust (Criterion), --rust-callgrind, --compare (Rust vs C++ table)
```

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

## The model

One step, two assets: bond `A(0) -> A(1)`, stock `S(0) -> S^u(1)` (prob. `p`) or `S^d(1)`.
Portfolio `(x, y)` = x shares + y bonds (negative = short sale / borrowing).

| Concept | Formula | C++ (Kotlin uses lowerCamelCase; Python and Rust snake_case) |
|---|---|---|
| Returns | `r = A(1)/A(0)-1`, `u`, `d` likewise for the stock | `r() u() d()` |
| Portfolio value, return | `V = xS + yA`, `K_V = (V(1)-V(0))/V(0)` | `Value0 Value1 PortfolioReturn` |
| Expected return, risk | `E(K)`, `sigma(K)` over the two states | `Expect StdDev` |
| No-arbitrage | `d < r < u` | `IsArbitrageFree`, `Arbitrage()` builds the strategy |
| Forwards | `F = S(0)(1+r)`, `V(1)=xS(1)+yA(1)+z(S(1)-F)` | `ForwardPrice`, `ForwardArbitrage`, `ImpliedForeignRate` |
| Replication | solve `xS^u+yA(1)=D^u`, `xS^d+yA(1)=D^d` | `Replicate`, `Price` |
| Risk-neutral pricing | `p* = (r-d)/(u-d)`, `D(0) = E*[D(1)]/(1+r)` | `RiskNeutralProb`, `RiskNeutralPrice` |
| Options | `(S-X)^+`, `(X-S)^+`, parity `C-P = S(0) - X/(1+r)` | `CallPayoff PutPayoff` |
| Hedged purchase | `S(1) - nC(1) + nC(0)(1+r)` | `HedgedPurchaseCost` |

## Worked examples (from `expected_output.txt`)

All of these come from the book, checked against the printed pages:

- **Example 1.1:** `A(0)=100, A(1)=110, S(0)=50, S(1)=52|48` gives `K_A=10%` and `K_S=±4%`.
- **Example 1.2:** a portfolio of 20 shares and 10 bonds has `V(0)=2000`, `V(1)=2140|2060` and `K_V=7%|3%`.
- **Exercise 1.1:** `A(0)=90, A(1)=100, S(0)=25, S(1)=30|20`. A portfolio of 10 shares and 15 bonds has `V(0)=1600`, `V(1)=1800|1700` and `K_V=12.5%|6.25%`.
- **Exercise 1.2:** the portfolio with `V(1)=1160|1040` is `x=12, y=8`, with `V(0)=1020`.
- **Example 1.1 market:** it is *not* arbitrage-free, because `u=4% < r=10%`. Shorting one share and buying 0.5 bonds costs 0 and pays 3 or 7 at time 1.
- **Exercise 1.3:** a currency cycle search (`fx_arbitrage`) over both dealers' quotes. USD→GBP (A)→EUR (B)→USD (A) turns 1 USD into 1.005941 USD.
- **Example 1.4 / Proposition 1.1:** `S(0)=100 → 125|105`, `A=100→110` gives `K_S=25%|5%`, and `105 < 110 < 125` holds, so the market is arbitrage-free with `p*=0.25`.
- **Proof of Proposition 1.1:** `Arbitrage()` builds the trades from the proof. When `A(1) ≤ S^d`, it borrows 100 and buys a share, `(x,y)=(1,-1)`. When `A(1) ≥ S^u`, it short-sells a share and invests the 100, `(-1,1)`. The report runs both cases on illustrative prices `S(1)=130|110` and `110|90`.
- **Section 1.4, risk and return:** `S(0)=80 → 100|60` with `p=0.8`, and $10,000 to invest. Mixed (50 shares, 60 bonds) gives `E=12%, σ=8%`. Bonds only gives `10%, 0`. Stock only gives `15%, 20%`.
- **Exercise 1.4:** a 50/50 split (`Split(10000, 0.5)`) is 62.5 shares and 50 bonds, with `V(1)=11750|9250`, `E(K_V)=12.5%` and `σ_V=10%`.
- **Section 1.5, forwards:** Example 1.5 (`F=80`) gives a long position `+4|-5` when `S(1)=84|75`. Proposition 1.2 gives `F=S(0)(1+r)=55`.
- **Exercise 1.5:** fair `F=38.08` against a quote of 38.60. The portfolio `(x,y,z)=(1,-0.34,-1)` costs 0 and pays 0.52 whatever `S(1)` is.
- **Exercise 1.6:** the implied sterling rate is 12%, so a bond paying £100 costs £89.2857.
- **Section 1.6, options:** `S(0)=100 → 120|80`, `A=100→110`. A call at X=100 replicates with `x=1/2, y=-4/11`, so `C(0)=13.6364`.
- **Exercise 1.7:** at X=90, `C(0)=20.4545`. At X=110, `C(0)=6.8182`.
- **Exercise 1.8:** with `A(1)=105`, `C(0)=11.9048`. With `A(1)=115`, `C(0)=15.2174`.
- **Exercise 1.9:** a put at X=100 gives `P(0)=4.5455`.
- Risk-neutral `p*=0.75` reproduces both prices, and put-call parity holds.

- **Section 1.7, managing risk:** with $1000, 10 shares give `1200|800`, while 73.3333 calls give `1466.67|0`.
- **Exercise 1.10:** a 50/50 split is 5 shares and 36.6667 calls, giving `1333.33|400`.
- **Hedging a future purchase:** `S(1)=160|40` with a call at X=100, `C(0)=31.8182`. With one call, the purchase costs `135|75`. With two calls it costs `110|110`, the same as a forward at 110.
- **Exercises 1.11 and 1.12:** at p=0.25, 0.5 and 0.75, σ is `51.96, 60, 51.96` without the option and `25.98, 30, 25.98` with it. The ratio is 1/2 for every p, and the tests check this at five values of p.

Every number in `expected_output.txt` now comes from the book's own examples and exercises.

## Build & run

```bash
./run_all.sh                     # everything, with cross-language diff

# C++
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch01
# Kotlin (JDK 21; the wrapper downloads Gradle 8.14 on first use)
cd kotlin && ./gradlew test run
# Python
cd python && python3 -m unittest one_step_test && python3 ch01.py
# Rust (1.86+)
cd rust && cargo test --release && cargo run --release
```

Formatting is 4 decimals with round-half-even on the exact double (`std::format`, Python f-strings,
`BigDecimal` in Kotlin, `{:.4}` in Rust) so the four outputs diff cleanly.

## Code style

The code follows the Google style guides for C++ and Python and the Android Kotlin style guide;
Rust, which has no Google guide, is rustfmt- and clippy-clean. See [STYLE.md](../STYLE.md). Run `./check_style.sh` to check, or `./check_style.sh --fix` to
reformat.
