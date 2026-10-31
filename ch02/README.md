# Chapter 2 — Risk-Free Assets

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 2. There are three
parallel implementations with the same API, and all three print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch02/
  cpp/      C++20, CMake        include/m4f/{interest,money_market}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch02/{Interest,MoneyMarket,Report}.kt, src/test/
  python/   Python 3.10+        interest.py, money_market.py, ch02.py, ch02_test.py
  run_all.sh                    build + test + run all three and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint (whichever are installed)
  bench.sh                      C++ microbenchmarks (Google Benchmark), --callgrind adds instruction counts
```

| Section | Formula | API (Kotlin; C++ uses PascalCase, Python snake_case) |
|---|---|---|
| 2.1.1 Simple interest | `V(t) = (1 + t r) P` | `simpleValue`, `simplePresentValue`, `simpleRate` |
| 2.1.2 Periodic compounding | `V(t) = (1 + r/m)^(tm) P`, `r_e = (1+r/m)^m - 1` | `periodicValue`, `periodicPresentValue`, `effectiveRate` |
| 2.1.3 Streams of payments | `PA = C (1 - (1+i)^-n) / i`, perpetuity `C / i` | `annuityFactor`, `annuityPresentValue`, `annuityFutureValue`, `perpetuityPresentValue`, `loanInstallment` |
| 2.1.4 Continuous compounding | `V(t) = e^(tr) P` | `continuousValue`, `continuousPresentValue`, `continuousEffectiveRate` |
| 2.1.5 Comparing methods | equal growth factors | `continuousFromPeriodic`, `periodicFromContinuous`, `periodicFromPeriodic` |
| 2.2.1 Zero-coupon bonds | `B(0,T) = (1+r/m)^(-Tm)` or `e^(-rT)` | `zeroCouponPrice`, `impliedRate` |
| 2.2.2 Coupon bonds | `Σ C(1+y)^-k + F(1+y)^-n` | `couponBondPrice`, `yieldToMaturity` (bisection) |
| 2.2.3 Money market account | `A(t) = A(0) e^(rt)`, rolling over zero-coupon bonds | `moneyMarketValue`, `moneyMarketRollover` |

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch02
cd kotlin && gradle test run          # or ./gradlew after `gradle wrapper`
cd python && python3 -m unittest ch02_test && python3 ch02.py
```

## Code style

The code follows the Google style guides for C++ and Python and the Android Kotlin style guide;
see [STYLE.md](../STYLE.md). Run `./check_style.sh` to check, or `./check_style.sh --fix` to
reformat.

## Benchmarks

`cpp/benchmarks/` holds Google Benchmark microbenchmarks for the C++ code. They are
off in a normal build; `./bench.sh` configures a separate Release build in
`cpp/build-bench` with `-DM4F_BUILD_BENCHMARKS=ON` (using an installed Google Benchmark,
or downloading v1.9.1) and runs them five times. `./bench.sh --callgrind` also runs
`cpp/benchmarks/callgrind_ir.py`, which reports exact instruction counts per iteration
under Valgrind; use those to compare versions on a noisy machine. Baseline numbers and
notes are in [`cpp/benchmarks/RESULTS.md`](cpp/benchmarks/RESULTS.md).
