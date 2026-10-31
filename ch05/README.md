# Chapter 5 — Portfolio Management

Code companion for *Mathematics for Finance* (Capiński & Zastawniak), Chapter 5. There are three
parallel implementations with the same API, and all three print the same output (`expected_output.txt`).

**The numbers in the report are illustrative.** They are not the book's own examples. The formulas follow the book.

```
ch05/
  cpp/      C++20, CMake        include/m4f/{risk,portfolio}.h (header-only), src/report.cc, tests/
  kotlin/   Kotlin 2.1, Gradle  src/main/kotlin/m4f/ch05/{Risk,Portfolio,Report}.kt, src/test/
  python/   Python 3.10+        risk.py, portfolio.py, ch05.py, ch05_test.py
  run_all.sh                    build + test + run all three and diff against expected_output.txt
  check_style.sh                clang-format, cpplint, pyink, pylint, ktlint (whichever are installed)
```

| Section | Formula | API (C++ / Kotlin; Python uses snake_case) |
|---|---|---|
| 5.1 Risk | `σ = sqrt(E(K - μ)²)` from scenario returns | `mean`, `stdDev`, `covariance`, `correlation` |
| 5.2.1 Two securities | `μ_V = w1μ1 + w2μ2`, `σ_V² = w1²σ1² + w2²σ2² + 2w1w2ρσ1σ2`; min variance `w1 = (σ2² - ρσ1σ2)/(σ1² + σ2² - 2ρσ1σ2)` | `twoAssetMean`, `twoAssetStd`, `minVarianceWeight` |
| 5.3.1 Several securities | `μ_V = w mᵀ`, `σ_V² = w C wᵀ` | `covarianceMatrix`, `portfolioMean`, `portfolioStd` |
| 5.3.2 Efficient frontier | MVP `w = uC⁻¹/(uC⁻¹uᵀ)`; minimum variance line for a target `μ_V` | `minVariancePortfolio`, `minVarianceLine` |
| 5.4.1 Capital market line | Market (tangency) portfolio `w_M ∝ (m - Ru)C⁻¹`; `μ = R + (μ_M - R)σ/σ_M` | `marketPortfolio`, `capitalMarketLine` |
| 5.4.2 Beta factor | `β_V = Cov(K_V, K_M)/σ_M²`, `β_V = Σ w_i β_i` | `beta` |
| 5.4.3 Security market line | `μ_V = R + (μ_M - R)β_V` | `securityMarketLine` |

The linear algebra (`dot`, `vecMat`, a Gauss-Jordan `inverse`) is written out by hand and is identical in
all three languages, so the printed results agree to the last digit.

## Build & run

```bash
./run_all.sh
cmake -S cpp -B cpp/build && cmake --build cpp/build -j && ctest --test-dir cpp/build && cpp/build/ch05
cd kotlin && ./gradlew test run
cd python && python3 -m unittest ch05_test && python3 ch05.py
```

## Code style

The code follows the Google style guides for C++ and Python and the Android Kotlin style guide;
see [STYLE.md](../STYLE.md). API names in this README are the Kotlin ones: C++ uses PascalCase
for functions (`riskNeutralProb` is `RiskNeutralProb`) and Python uses snake_case. Run
`./check_style.sh` to check, or `./check_style.sh --fix` to reformat.
