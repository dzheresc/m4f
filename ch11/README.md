# Chapter 11 — Stochastic Interest Rates

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 11. There are three
parallel implementations with the same API, and all three print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch11/
  cpp/      C++20, CMake        include/m4f/{rate_tree,derivatives}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch11/{RateTree,Derivatives,Report}.kt, src/test/
  python/   Python 3.10+        rate_tree.py, derivatives.py, ch11.py, ch11_test.py
  run_all.sh                    build + test + run all three and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint (whichever are installed)
```

The short rate lives on a recombining binomial lattice: node `(n,k)` after `n` steps of length `τ`, `k` of them
up. `r(n,k)` applies over `[n, n+1]`, so `B(n,n+1;k) = e^(-r(n,k)τ)`. Everything else is backward induction with
risk-neutral probabilities `p*(n,k)` (a lattice too, constant in the report):
`V(n,k) = e^(-r(n,k)τ) [ p* V(n+1,k+1) + (1-p*) V(n+1,k) + payment(n,k) ]`, optionally with early exercise.

| Section | Formula | API (C++ / Kotlin; Python uses snake_case) |
|---|---|---|
| 11.1 Binomial tree model | Short-rate lattice, one-step bonds, money market account `A(n+1) = A(n) e^(r τ)` along a path | `rateLattice`, `oneStepPrice`, `moneyMarket` |
| 11.2 Arbitrage pricing of bonds | `B(n,N)` by backward induction from `B(N,N)=1`; term structure `y(n,N) = -ln B/((N-n)τ)` at every node; `E*[B(n+1,N)] = B(n,N) e^(rτ)` | `discountBack`, `bondLattice`, `bondPrices`, `yieldRate` |
| 11.2.1 Risk-neutral probabilities | `p*(n,k) = (B(n,N;k)e^(rτ) - B(n+1,N;k)) / (B(n+1,N;k+1) - B(n+1,N;k))`, the same for every maturity; a mispriced bond implies an inconsistent `p*` (arbitrage) | `impliedProb`, `impliedProbs` |
| 11.3.1 Options | European and American calls/puts on a zero-coupon bond, payoff at expiry `n_exp` then discounted | `bondOption` |
| 11.3.2 Swaps | Floating `L(n,k) = e^(r(n,k)τ) - 1` paid at `n+1`; swap rate `K = (1 - B(0,N)) / Σ B(0,n)`; payer swap on the tree | `swapRate`, `payerSwap`, `pricePayments` |
| 11.3.3 Caps and floors | Caplet `max(L - K, 0)`, floorlet `max(K - L, 0)`; `cap - floor = payer swap`; cap = sum of caplets | `cap`, `floor`, `caplet` |

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch11
cd kotlin && ./gradlew test run
cd python && python3 -m unittest ch11_test && python3 ch11.py
```

## Code style

The code follows the Google style guides for C++ and Python and the Android Kotlin style guide;
see [STYLE.md](../STYLE.md). API names in this README are the Kotlin ones: C++ uses PascalCase
for functions (`riskNeutralProb` is `RiskNeutralProb`) and Python uses snake_case. Run
`./check_style.sh` to check, or `./check_style.sh --fix` to reformat.
