# Chapter 4 — Discrete Time Market Models

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 4. There are three
parallel implementations with the same API, and all three print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch04/
  cpp/      C++20, CMake        include/m4f/{strategies,no_arbitrage}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch04/{Strategies,NoArbitrage,Report}.kt, src/test/
  python/   Python 3.10+        strategies.py, no_arbitrage.py, ch04.py, ch04_test.py
  run_all.sh                    build + test + run all three and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint (whichever are installed)
```

Positions follow the book's timing: `x(n), y(n)` are chosen at time `n-1` and held over `(n-1, n]`.
In code, `x[0]` is `x(1)`.

| Section | Formula | API (C++ / Kotlin; Python uses snake_case) |
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
```

## Code style

The code follows the Google style guides for C++ and Python and the Android Kotlin style guide;
see [STYLE.md](../STYLE.md). API names in this README are the Kotlin ones: C++ uses PascalCase
for functions (`riskNeutralProb` is `RiskNeutralProb`) and Python uses snake_case. Run
`./check_style.sh` to check, or `./check_style.sh --fix` to reformat.
