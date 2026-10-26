# Chapter 3 — Risky Assets

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 3. There are three
parallel implementations with the same API, and all three print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch03/
  cpp/      C++20, CMake        include/m4f/{returns,binomial_tree,other_models}.hpp (header-only), src/report.cpp, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch03/{Returns,BinomialTree,OtherModels,Report}.kt, src/test/
  python/   Python 3.10+        returns.py, binomial_tree.py, other_models.py, ch03.py, test_ch03.py
  run_all.sh                    build + test + run all three and diff against expected_output.txt
```

| Section | Formula | API (C++ / Kotlin; Python uses snake_case) |
|---|---|---|
| 3.1.1 Return | `K(n) = (S(n) - S(n-1)) / S(n-1)`, `k(n) = ln(S(n)/S(n-1))`, `1+K(0,n) = Π(1+K(i))`, `k(0,n) = Σ k(i)` | `oneStepReturn` (optional dividend), `logReturn`, `returns`, `logReturns`, `compoundReturn` |
| 3.1.2 Expected return | `E(S(1)) = (1 + E(K)) S(0)`; independent steps multiply `1 + E(K)` | `expectation`, `variance` |
| 3.2 Binomial tree | `S(n) = S(0)(1+u)^k(1+d)^(n-k)` with prob. `C(n,k) p^k (1-p)^(n-k)` | `BinomialTree{S0,u,d,r}`: `price`, `expectedReturn`, `expectedPrice`; `binomialProb` |
| 3.2.1 Risk-neutral probability | `d < r < u`, `p* = (r-d)/(u-d)`, `E*(S(n)) = (1+r)^n S(0)` | `isArbitrageFree`, `riskNeutralProb` |
| 3.2.2 Martingale property | `E*(S~(n+1) \| S(n)) = S~(n)`, `S~(n) = (1+r)^-n S(n)` | `discountedPrice`, `conditionalDiscounted` |
| 3.3.1 Trinomial tree | `p_u u + p_m m + p_d d = r`: a one-parameter family of risk-neutral probabilities | `TrinomialModel{u,m,d,r}`: `riskNeutralProbs(pm)`, `maxMiddleProb` |
| 3.3.2 Continuous-time limit | `k = mτ ± σ√τ` w.p. ½ → `ln S(T) ~ N(ln S(0) + mT, σ²T)`, `E S(T) = S(0) e^((m+σ²/2)T)` | `binomialExpectedPrice`, `lognormalExpectedPrice`, `binomialProbAbove`, `lognormalProbAbove`, `normCdf` |

`P(S(T) > K)` in 3.3.2 converges with a visible lattice wobble (0.3822 at N=100, 0.4124 at N=1000, 0.4090 at
N=10000, limit 0.4104), because the binomial distribution is discrete and the strike falls between nodes.

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch03
cd kotlin && ./gradlew test run
cd python && python3 -m unittest test_ch03 && python3 ch03.py
```
