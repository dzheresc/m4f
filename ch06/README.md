# Chapter 6 — Forward and Futures Contracts

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 6. There are three
parallel implementations with the same API, and all three print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch06/
  cpp/      C++20, CMake        include/m4f/{forwards,futures}.hpp (header-only), src/report.cpp, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch06/{Forwards,Futures,Report}.kt, src/test/
  python/   Python 3.10+        forwards.py, futures.py, ch06.py, test_ch06.py
  run_all.sh                    build + test + run all three and diff against expected_output.txt
```

Forwards use continuous compounding, as the chapter does. Futures pricing uses the binomial tree with `r` per step.

| Section | Formula | API (C++ / Kotlin; Python uses snake_case) |
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
cd python && python3 -m unittest test_ch06 && python3 ch06.py
```
