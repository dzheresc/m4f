# Chapter 8 — Option Pricing

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 8. There are three
parallel implementations with the same API, and all three print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch08/
  cpp/      C++20, CMake        include/m4f/{binomial_pricing,black_scholes}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch08/{BinomialPricing,BlackScholes,Report}.kt, src/test/
  python/   Python 3.10+        binomial_pricing.py, black_scholes.py, ch08.py, ch08_test.py
  run_all.sh                    build + test + run all three and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint (whichever are installed)
  bench.sh                      C++ microbenchmarks (Google Benchmark), --callgrind adds instruction counts
```

Sections 8.1–8.2 use the book's binomial tree with returns `u`, `d` and `r` per step. Section 8.3 switches to
continuous compounding and the CRR parametrisation `1+u = e^(σ√τ)`, `1+d = e^(-σ√τ)`, `1+r = e^(rτ)`.

| Section | Formula | API (C++ / Kotlin; Python uses snake_case) |
|---|---|---|
| 8.1.1 One step | Replicate `C^u`, `C^d` with `x = (C^u - C^d)/(S(0)(u-d))` shares; `C(0) = E*(C(1))/(1+r)` | `oneStepReplication`, `riskNeutralProb` |
| 8.1.2 Two steps | Backward induction, and the replicating `(x, y)` at each node | `optionTree`, `replicatingPosition` |
| 8.1.3 General N-step model | `C(0) = (1+r)^-N Σ C(N,k) p*^k (1-p*)^(N-k) f(S(N,k))` for any payoff `f` | `europeanPriceSum`, `optionTree` |
| 8.1.4 Cox-Ross-Rubinstein formula | `C = S(0)Φ(m;N,p') - X(1+r)^-N Φ(m;N,p*)`, `p' = p*(1+u)/(1+r)` | `crrCall`, `binomialSum` |
| 8.2 American options | `V(n) = max(payoff, (p*V^u + (1-p*)V^d)/(1+r))`; early-exercise nodes | `optionTree(..., american=true)`, `earlyExerciseNodes` |
| 8.3 Black-Scholes | `C = SN(d1) - Xe^(-rT)N(d2)`, `P = Xe^(-rT)N(-d2) - SN(-d1)`, `Δ = N(d1)`; the CRR tree converges to it | `bsCall`, `bsPut`, `bsCallDelta`, `normCdf`, `crrTreePrice` |

`normCdf` is Hart's double-precision rational approximation (as given by G. West), so the Black-Scholes values
agree across languages to the last printed digit.

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch08
cd kotlin && ./gradlew test run
cd python && python3 -m unittest ch08_test && python3 ch08.py
```

## Code style

The code follows the Google style guides for C++ and Python and the Android Kotlin style guide;
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
