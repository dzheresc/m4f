# Chapter 10 — Variable Interest Rates

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 10. There are four
parallel implementations with the same API, and all four print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch10/
  cpp/      C++20, CMake        include/m4f/{bonds,term_structure}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch10/{Bonds,TermStructure,Report}.kt, src/test/
  python/   Python 3.10+        bonds.py, term_structure.py, ch10.py, ch10_test.py
  rust/     Rust 2024, Cargo    src/{bonds,term_structure,report}.rs, tests/, benches/
  run_all.sh                    build + test + run all four and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint, rustfmt + clippy (whichever are installed)
  bench.sh                      C++ microbenchmarks (Google Benchmark), --callgrind adds instruction counts;
                                --rust (Criterion), --rust-callgrind, --compare (Rust vs C++ table)
```

All rates are continuously compounded. A bond is a list of cash flows `(t_i, C_i)`.

| Section | Formula | API (C++ / Kotlin; Python and Rust use snake_case) |
|---|---|---|
| 10.1.1 Investment in single bonds | One yield `y` for all maturities, `P = Σ C_i e^(-y t_i)`; after a jump in `y` the value at horizon `t` is coupons reinvested at `y` plus the remaining flows discounted at `y` | `couponBond`, `zeroCoupon`, `price`, `investmentValue`, `logReturn` |
| 10.1.2 Duration | `D = Σ t_i C_i e^(-y t_i) / P`, `dP/dy = -D P`; the value at horizon `t = D` is immunised to first order | `duration` |
| 10.1.3 Portfolios of bonds | Portfolio duration is the value-weighted average; `w_1 D_1 + w_2 D_2 = D_target`, `w_1 + w_2 = 1` | `immunisingWeights`, `immunise`, `value` |
| 10.1.4 Dynamic hedging | Re-immunise at each step so the duration equals the remaining horizon, collecting coupons into cash; compared with a static portfolio | `dynamicHedge` |
| 10.2.1 Forward rates | `y(0,T) = -ln B(0,T)/T`, `f(0,T_1,T_2) = (ln B(0,T_1) - ln B(0,T_2))/(T_2-T_1)`, `B(0,T_n) = exp(-Σ f Δt)`, implied `B(t,T) = B(0,T)/B(0,t)`, FRA replication, coupon bond under the term structure and its yield | `yieldRate`, `forwardRate(s)`, `pricesFromForwardRates`, `impliedPrice`, `couponBondPrice`, `yieldToMaturity` |
| 10.2.2 Money market account | Rolling over one-period bonds at the realised (= forward) rates gives `A(T_n) = Π 1/B(T_{i-1},T_i) = 1/B(0,T_n)` | `moneyMarketAccount` |

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch10
cd kotlin && ./gradlew test run
cd python && python3 -m unittest ch10_test && python3 ch10.py
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
