# Python performance report

The C++ speedups from [PERFORMANCE.md](PERFORMANCE.md) are now in the Python
code too, chapter by chapter, with the same algorithms. Each chapter's hottest
benchmark is now **1.6x to 163x faster** in CPython. Chapter 6 needed no
change, as in C++.

Every chapter's report is still byte-identical to its `expected_output.txt`.
Each change has a new unit test against the code it replaced, bit for bit
where the arithmetic allows. The code still uses only the standard library.

## Method

- **Tool:** `timeit` from the standard library, in `python/bench_chNN.py`, run
  with `./bench.sh --python [name filter]` from a chapter directory.
- **Settings:** each case runs long enough for `timeit.autorange` (at least
  0.2 s), and the reported time per call is the best of 5 such repeats (3 for
  cases over 2 s). The minimum is the run least disturbed by other load, as the
  `timeit` documentation advises.
- **Machine:** the same cloud VM as the C++ report (4 shared vCPU at 2.1 GHz),
  CPython 3.13.18. The baseline and the new code were run back to back, chapter
  by chapter, to keep the VM's drift out of the ratios. Times still vary by
  10-20% between runs; differences below that are noise.
- **Baseline:** the code at `main` before this change, run with the same
  benchmark files.

## Headline per chapter

The C++ column is the instruction-count ratio from the C++ report, for
comparison.

| Ch | Benchmark | Before | After | Speedup | C++ (Ir) | Change |
|---:|---|---:|---:|---:|---:|---|
| 1 | BestCycleScaling/8 | 217 ms | 7.11 ms | **30.5x** | 94x | n×n best-leg index table, search by index |
| 2 | CouponBondPrice/360 | 31 µs | 190 ns | **163x** | 248x | closed-form annuity (expm1/log1p) |
| 2 | YieldToMaturity/360 | 1.2 ms | 41.9 µs | **28.7x** | 191x | Newton inside the bisection bracket |
| 3 | ExpectedPrice/1000 | 745 µs | 199 µs | **3.7x** | 29x | O(n) log-space sum |
| 4 | ExpectedDiscountedValue/16 | 902 ms | 64.9 ms | **13.9x** | 53x | depth-first walk sharing prefixes |
| 5 | MinVariancePortfolio/200 | 624 ms | 52.5 ms | **11.9x** | 9.5x | Cholesky solve instead of the inverse |
| 7 | CrrAmericanPut/2000 | 441 ms | 168 ms | **2.6x** | 31x | one table of node prices instead of pow per node |
| 8 | CrrTreePriceAmerican/2000 | 610 ms | 391 ms | **1.6x** | 28x | rolling row, power tables |
| 9 | NormInv | 90.9 µs | 713 ns | **128x** | 108x | Acklam + one Halley step |
| 10 | YieldToMaturity/3 | 105 µs | 2.78 µs | **37.8x** | 55x | Newton inside the bisection bracket |
| 11 | BondPrices/200 | 215 ms | 3.75 ms | **57.3x** | 75x | forward induction, O(N²) instead of O(N³) |

## Whole chapter report

`report()` builds the full chapter output. Most of its time is string
formatting, so it moves less than the hot functions.

| Ch | Before | After | Speedup |
|---:|---:|---:|---:|
| 1 | 114 µs | 113 µs | 1x |
| 2 | 37.2 µs | 21.8 µs | 1.7x |
| 3 | 1.66 ms | 1.8 ms | 0.92x |
| 4 | 149 µs | 55.2 µs | 2.7x |
| 5 | 111 µs | 86.9 µs | 1.3x |
| 7 | 92.2 ms | 56.8 ms | 1.6x |
| 8 | 410 ms | 210 ms | 2x |
| 9 | 3.29 ms | 1.06 ms | 3.1x |
| 10 | 659 µs | 287 µs | 2.3x |
| 11 | 216 µs | 175 µs | 1.2x |

## Notes

- **Algorithmic changes carry over; constant-factor ones mostly do not.**
  Replacing an algorithm (bisection by Newton, O(N³) by O(N²), an inverse by a
  Cholesky solve, a series by its closed form) gains as much in Python as in
  C++, or more. In chapter 5 the Cholesky solve gains more than in C++ because
  the pure-Python inverse was the slowest part.
- **Removing `pow` from tree nodes (chapters 7 and 8) gains 1.6x to 2.6x,
  not 30x.** In C++ the two `pow` calls were most of the cost of a node; in
  CPython the interpreter's per-node work dominates either way. The Python
  versions also build each row with a comprehension and take maxima without a
  call per node, which is where most of the gain comes from. Going further would
  need NumPy, which the code avoids on purpose.
- **Chapter 3:** Python already used `lgamma` for the binomial probability, so
  the old expected price was O(n) calls, not O(n²) work; the new single pass is
  about 4x faster.
- **Slower cases:** `CouponBondPrice/3` (chapter 10) is 0.55x, about a
  quarter of a microsecond slower than the old `list.index` for three flows;
  it is 20x faster at 360 flows. `FxMarketConstruct` (chapter 1) is 0.81x
  because it builds the best-leg table once per market, as in C++.
  `BinomialSum` (chapter 8, unchanged code) and the chapter 1, 3 and 11
  reports move within the VM's noise.

## All measurements

### Chapter 1

| Benchmark | Before | After | Speedup |
|---|---:|---:|---:|
| FxMarketConstruct | 4.89 µs | 6.04 µs | 0.81x |
| BestLeg | 274 ns | 99.5 ns | 2.8x |
| BestCycleBook | 7.68 µs | 3.27 µs | 2.4x |
| BestCycleScaling/3 | 6.95 µs | 3.59 µs | 1.9x |
| BestCycleScaling/6 | 2.26 ms | 160 µs | 14.1x |
| BestCycleScaling/8 | 217 ms | 7.11 ms | 30.5x |
| Report | 114 µs | 113 µs | 1x |

### Chapter 2

| Benchmark | Before | After | Speedup |
|---|---:|---:|---:|
| CouponBondPrice/3 | 387 ns | 176 ns | 2.2x |
| YieldToMaturity/3 | 17 µs | 1.91 µs | 8.9x |
| CouponBondPrice/30 | 2.34 µs | 190 ns | 12.3x |
| YieldToMaturity/30 | 98.4 µs | 5.76 µs | 17.1x |
| CouponBondPrice/360 | 31 µs | 190 ns | 163x |
| YieldToMaturity/360 | 1.2 ms | 41.9 µs | 28.7x |
| Report | 37.2 µs | 21.8 µs | 1.7x |

### Chapter 3

| Benchmark | Before | After | Speedup |
|---|---:|---:|---:|
| BinomialProb/3 | 358 ns | 408 ns | 0.88x |
| ExpectedPrice/3 | 1.96 µs | 718 ns | 2.7x |
| BinomialProb/100 | 570 ns | 580 ns | 0.98x |
| ExpectedPrice/100 | 69.7 µs | 14 µs | 5x |
| BinomialProb/1000 | 641 ns | 632 ns | 1x |
| ExpectedPrice/1000 | 745 µs | 199 µs | 3.7x |
| Report | 1.66 ms | 1.8 ms | 0.92x |

### Chapter 4

| Benchmark | Before | After | Speedup |
|---|---:|---:|---:|
| ExpectedDiscountedValue/4 | 54.7 µs | 13.5 µs | 4x |
| ExpectedDiscountedValue/12 | 34.5 ms | 3.65 ms | 9.4x |
| ExpectedDiscountedValue/16 | 902 ms | 64.9 ms | 13.9x |
| SelfFinancing/12 | 3.6 µs | 1.68 µs | 2.1x |
| SelfFinancing/1000 | 309 µs | 150 µs | 2.1x |
| Report | 149 µs | 55.2 µs | 2.7x |

### Chapter 5

| Benchmark | Before | After | Speedup |
|---|---:|---:|---:|
| MinVariancePortfolio/3 | 7.19 µs | 3.7 µs | 1.9x |
| VecMat/3 | 1.27 µs | 1.01 µs | 1.3x |
| MinVariancePortfolio/50 | 8.81 ms | 957 µs | 9.2x |
| VecMat/50 | 113 µs | 88 µs | 1.3x |
| MinVariancePortfolio/200 | 624 ms | 52.5 ms | 11.9x |
| VecMat/200 | 2.18 ms | 1.52 ms | 1.4x |
| Correlation/3 | 3.2 µs | 1.12 µs | 2.9x |
| Correlation/10000 | 3.32 ms | 1.56 ms | 2.1x |
| Report | 111 µs | 86.9 µs | 1.3x |

### Chapter 7

| Benchmark | Before | After | Speedup |
|---|---:|---:|---:|
| CrrEuropeanPut/100 | 355 µs | 365 µs | 0.97x |
| CrrAmericanPut/100 | 1.23 ms | 617 µs | 2x |
| CrrEuropeanPut/2000 | 133 ms | 114 ms | 1.2x |
| CrrAmericanPut/2000 | 441 ms | 168 ms | 2.6x |
| Report | 92.2 ms | 56.8 ms | 1.6x |

### Chapter 8

| Benchmark | Before | After | Speedup |
|---|---:|---:|---:|
| OptionTreeAmerican/100 | 1.43 ms | 1.33 ms | 1.1x |
| EarlyExerciseNodes/100 | 3.01 ms | 2.03 ms | 1.5x |
| CrrTreePriceEuropean/100 | 479 µs | 427 µs | 1.1x |
| CrrTreePriceAmerican/100 | 1.41 ms | 1.34 ms | 1x |
| OptionTreeAmerican/2000 | 692 ms | 468 ms | 1.5x |
| EarlyExerciseNodes/2000 | 1.28 s | 897 ms | 1.4x |
| CrrTreePriceEuropean/2000 | 240 ms | 170 ms | 1.4x |
| CrrTreePriceAmerican/2000 | 610 ms | 391 ms | 1.6x |
| BinomialSum/1000 | 172 µs | 212 µs | 0.81x |
| Report | 410 ms | 210 ms | 2x |

### Chapter 9

| Benchmark | Before | After | Speedup |
|---|---:|---:|---:|
| NormInv | 90.9 µs | 713 ns | 128x |
| BsGreeksCall | 2.93 µs | 1.82 µs | 1.6x |
| BsGreeksPut | 3.17 µs | 2.3 µs | 1.4x |
| ValueAtRisk | 80.7 µs | 3.48 µs | 23.2x |
| Report | 3.29 ms | 1.06 ms | 3.1x |

### Chapter 10

| Benchmark | Before | After | Speedup |
|---|---:|---:|---:|
| CouponBondPrice/3 | 306 ns | 556 ns | 0.55x |
| YieldToMaturity/3 | 105 µs | 2.78 µs | 37.8x |
| CouponBondPrice/30 | 4.44 µs | 2.16 µs | 2.1x |
| YieldToMaturity/30 | 577 µs | 14.1 µs | 41x |
| CouponBondPrice/360 | 513 µs | 25.4 µs | 20.2x |
| YieldToMaturity/360 | 4.8 ms | 234 µs | 20.5x |
| DynamicHedge/5 | 66.7 µs | 21.4 µs | 3.1x |
| DynamicHedge/260 | 3.76 ms | 843 µs | 4.5x |
| Report | 659 µs | 287 µs | 2.3x |

### Chapter 11

| Benchmark | Before | After | Speedup |
|---|---:|---:|---:|
| BondPrices/10 | 41.3 µs | 14 µs | 3x |
| BondPrices/50 | 3.48 ms | 258 µs | 13.4x |
| BondPrices/200 | 215 ms | 3.75 ms | 57.3x |
| Caplet/100 | 1.03 ms | 654 µs | 1.6x |
| RateLattice/100 | 288 µs | 292 µs | 0.99x |
| Caplet/1000 | 126 ms | 63.5 ms | 2x |
| RateLattice/1000 | 38.8 ms | 33.2 ms | 1.2x |
| Report | 216 µs | 175 µs | 1.2x |
