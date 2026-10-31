# Chapter 4 — Discrete Time Market Models

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 4. There are four
parallel implementations with the same API, and all four print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch04/
  cpp/      C++20, CMake        include/m4f/{strategies,no_arbitrage}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch04/{Strategies,NoArbitrage,Report}.kt, src/test/
  python/   Python 3.10+        strategies.py, no_arbitrage.py, ch04.py, ch04_test.py
  rust/     Rust 2024, Cargo    src/{strategies,no_arbitrage,report}.rs, tests/, benches/
  run_all.sh                    build + test + run all four and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint, rustfmt + clippy (whichever are installed)
  bench.sh                      C++ microbenchmarks (Google Benchmark), --callgrind adds instruction counts;
                                --rust (Criterion), --rust-callgrind, --compare (Rust vs C++ table)
```

Positions follow the book's timing: `x(n), y(n)` are chosen at time `n-1` and held over `(n-1, n]`.
In code, `x[0]` is `x(1)`.

| Section | Formula | API (C++ / Kotlin; Python and Rust use snake_case) |
|---|---|---|
| 4.1.1 Investment strategies | `V(n) = x(n)S(n) + y(n)A(n)`, self-financing `x(n+1)S(n) + y(n+1)A(n) = V(n)`; `V(N)-V(0) = Σ x ΔS + y ΔA`; `V~(N)-V~(0) = Σ x ΔS~` | `selfFinancing`, `gains`, `discountedGains` |
| 4.1.2 No arbitrage | `V(0) = 0, V(1) ≥ 0`, `V(1) > 0` with positive probability is impossible iff `min K < r < max K` | `isArbitrageFree`, `arbitrageStrategy`, `oneStepValues` |
| 4.1.3 Binomial tree | Discounted values of self-financing strategies are martingales under `p*`: `E*(V~(N)) = V(0)` | `expectedDiscountedValue(s0,u,d,r,numSteps,v0,rule,p)`, enumerating all `2^N` paths |
| 4.1.4 FTAP | No arbitrage ⇔ there is a risk-neutral `q > 0` with `Σ q_i K_i = r` | `riskNeutralProbs` (constructs one `q`, or none) |
| 4.2 Extended models | Several stocks with dividends: `V(n) = Σ x_j(n)(S_j(n) + div_j(n)) + y(n)A(n)` | `selfFinancingMulti` |

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch04
cd kotlin && ./gradlew test run
cd python && python3 -m unittest ch04_test && python3 ch04.py
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
