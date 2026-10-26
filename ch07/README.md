# Chapter 7 — Options: General Properties

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 7. There are three
parallel implementations with the same API, and all three print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch07/
  cpp/      C++20, CMake        include/m4f/{options,binomial}.hpp (header-only), src/report.cpp, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch07/{Options,Binomial,Report}.kt, src/test/
  python/   Python 3.10+        options.py, binomial.py, ch07.py, test_ch07.py
  run_all.sh                    build + test + run all three and diff against expected_output.txt
```

Interest is compounded continuously. The model prices used in 7.3–7.5 come from a 200-step Cox-Ross-Rubinstein
tree (`crrPrice`; σ = 0.2, r = 0.05), which Chapter 8 develops properly.

| Section | Formula | API (C++ / Kotlin; Python uses snake_case) |
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
cd python && python3 -m unittest test_ch07 && python3 ch07.py
```
