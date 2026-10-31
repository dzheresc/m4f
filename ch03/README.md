# Chapter 3 — Risky Assets

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 3. There are four
parallel implementations with the same API, and all four print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch03/
  cpp/      C++20, CMake        include/m4f/{returns,binomial_tree,other_models}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch03/{Returns,BinomialTree,OtherModels,Report}.kt, src/test/
  python/   Python 3.10+        returns.py, binomial_tree.py, other_models.py, ch03.py, ch03_test.py
  rust/     Rust 2024, Cargo    src/{returns,binomial_tree,other_models,report}.rs, tests/, benches/
  run_all.sh                    build + test + run all four and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint, rustfmt + clippy (whichever are installed)
  bench.sh                      C++ microbenchmarks (Google Benchmark), --callgrind adds instruction counts;
                                --rust (Criterion), --rust-callgrind, --compare (Rust vs C++ table)
```

| Section | Formula | API (C++ / Kotlin; Python uses snake_case) |
|---|---|---|
| 3.1.1 Return | `K(n) = (S(n) - S(n-1)) / S(n-1)`, `k(n) = ln(S(n)/S(n-1))`, `1+K(0,n) = Π(1+K(i))`, `k(0,n) = Σ k(i)` | `oneStepReturn` (optional dividend), `logReturn`, `returns`, `logReturns`, `compoundReturn` |
| 3.1.2 Expected return | `E(S(1)) = (1 + E(K)) S(0)`; independent steps multiply `1 + E(K)` | `expectation`, `variance` |
| 3.2 Binomial tree | `S(n) = S(0)(1+u)^k(1+d)^(n-k)` with prob. `C(n,k) p^k (1-p)^(n-k)` | `BinomialTree{s0,u,d,r}`: `price`, `expectedReturn`, `expectedPrice`; `binomialProb` |
| 3.2.1 Risk-neutral probability | `d < r < u`, `p* = (r-d)/(u-d)`, `E*(S(n)) = (1+r)^n S(0)` | `isArbitrageFree`, `riskNeutralProb` |
| 3.2.2 Martingale property | `E*(S~(n+1) \| S(n)) = S~(n)`, `S~(n) = (1+r)^-n S(n)` | `discountedPrice`, `conditionalDiscounted` |
| 3.3.1 Trinomial tree | `p_u u + p_m m + p_d d = r`: a one-parameter family of risk-neutral probabilities | `TrinomialModel{u,m,d,r}`: `riskNeutralProbs(pMid)`, `maxMiddleProb` |
| 3.3.2 Continuous-time limit | `k = mτ ± σ√τ` w.p. ½ → `ln S(T) ~ N(ln S(0) + mT, σ²T)`, `E S(T) = S(0) e^((m+σ²/2)T)` | `binomialExpectedPrice`, `lognormalExpectedPrice`, `binomialProbAbove`, `lognormalProbAbove`, `normCdf` |

`P(S(T) > K)` in 3.3.2 converges with a visible lattice wobble (0.3822 at N=100, 0.4124 at N=1000, 0.4090 at
N=10000, limit 0.4104), because the binomial distribution is discrete and the strike falls between nodes.

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch03
cd kotlin && ./gradlew test run
cd python && python3 -m unittest ch03_test && python3 ch03.py
cd rust && cargo test --release && cargo run --release   # Rust 1.86+
```

## Code style

The code follows the Google style guides for C++ and Python and the Android Kotlin style guide;
Rust, which has no Google guide, is rustfmt- and clippy-clean;
see [STYLE.md](../STYLE.md). API names in this README are the Kotlin ones: C++ uses PascalCase
for functions (`riskNeutralProb` is `RiskNeutralProb`) and Python uses snake_case. Run
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
