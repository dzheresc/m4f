# Kotlin performance report

The C++ speedups from [PERFORMANCE.md](PERFORMANCE.md) are now in the Kotlin
code too, chapter by chapter, with the same algorithms. Each chapter's hottest
benchmark is now **3.5x to 355x faster** on the JVM, and most of them no
longer allocate. Chapter 6 needed no change, as in C++.

Every chapter's report is still byte-identical to its `expected_output.txt`.
Each change has a new unit test against the code it replaced, bit for bit
where the arithmetic allows.

## Method

- **Tool:** [JMH](https://github.com/openjdk/jmh) 1.36 through the
  `me.champeau.jmh` Gradle plugin. Benchmarks live in `kotlin/src/jmh` and run
  with `./bench.sh --kotlin [regex]` from a chapter directory.
- **Settings:** average time per call, 3 warm-up and 5 measured iterations of
  1 s, one fork. The GC profiler adds bytes allocated per call
  (`gc.alloc.rate.norm`), which is close to deterministic and so a useful
  second metric next to time.
- **Machine:** the same cloud VM as the C++ report (4 shared vCPU at 2.1 GHz),
  OpenJDK 21.0.12, Kotlin 2.1.21. Times on the shared VM vary by up to about
  10% between runs; differences below that are noise.
- **Baseline:** the code at `main` before this change, run with the same
  benchmark files.

## Headline per chapter

The C++ column is the instruction-count ratio from the C++ report, for
comparison.

| Ch | Benchmark | Before | After | Speedup | Alloc before → after | C++ (Ir) | Change |
|---:|---|---:|---:|---:|---|---:|---|
| 1 | BestCycleScaling/8 | 30.8 ms | 256 µs | **120x** | 1.52 KB → 744 B | 94x | n×n best-leg index table, search by index |
| 2 | CouponBondPrice/360 | 5.42 µs | 49.8 ns | **109x** | 0 B → 0 B | 248x | closed-form annuity (expm1/log1p) |
| 2 | YieldToMaturity/360 | 238 µs | 1.39 µs | **171x** | 0 B → 0 B | 191x | Newton inside the bisection bracket |
| 3 | ExpectedPrice/1000 | 10.2 ms | 28.7 µs | **355x** | 4 B → 0 B | 29x | O(n) log-space sum |
| 4 | ExpectedDiscountedValue/16 | 115 ms | 4.52 ms | **25.5x** | 211 MB → 282 B | 53x | depth-first walk sharing prefixes |
| 5 | MinVariancePortfolio/200 | 3.79 ms | 1.09 ms | **3.5x** | 2.57 MB → 333 KB | 9.5x | Cholesky solve instead of the inverse |
| 7 | CrrAmericanPut/2000 | 85.8 ms | 2.82 ms | **30.4x** | 16.1 KB → 48 KB | 31x | one table of node prices instead of pow per node |
| 8 | CrrTreePriceAmerican/2000 | 396 ms | 6.14 ms | **64.4x** | 78.2 MB → 48.1 KB | 28x | rolling row, power tables |
| 9 | NormInv | 2.19 µs | 33.1 ns | **66.2x** | 0 B → 0 B | 108x | Acklam + one Halley step |
| 10 | YieldToMaturity/3 | 3.97 µs | 126 ns | **31.6x** | 0 B → 0 B | 55x | Newton inside the bisection bracket |
| 11 | BondPrices/200 | 24 ms | 160 µs | **150x** | 51.4 MB → 4.88 KB | 75x | forward induction, O(N²) instead of O(N³) |

## Whole chapter report

`report()` builds the full chapter output. Most of its time is string
formatting, so it moves less than the hot functions.

| Ch | Before | After | Speedup | Alloc before → after |
|---:|---:|---:|---:|---|
| 1 | 55.2 µs | 55.4 µs | 1x | 159 KB → 159 KB |
| 2 | 26 µs | 25.1 µs | 1x | 71 KB → 69.7 KB |
| 3 | 364 µs | 360 µs | 1x | 108 KB → 108 KB |
| 4 | 197 µs | 20.9 µs | 9.4x | 100 KB → 65.7 KB |
| 5 | 47.1 µs | 39.5 µs | 1.2x | 137 KB → 124 KB |
| 7 | 9.59 ms | 1.28 ms | 7.5x | 187 KB → 299 KB |
| 8 | 69 ms | 3.61 ms | 19.1x | 75.1 MB → 223 KB |
| 9 | 240 µs | 166 µs | 1.4x | 397 KB → 398 KB |
| 10 | 117 µs | 98.9 µs | 1.2x | 305 KB → 295 KB |
| 11 | 105 µs | 93.3 µs | 1.1x | 266 KB → 248 KB |

## Notes

- **Kotlin gains more than C++ where the old code boxed.** `List<Double>`
  stores boxed values, so the old chapter 4 path walk allocated 211 MB per call
  at N = 16 and the chapter 11 zero curve 51 MB at N = 200. Both now allocate a
  few hundred bytes. Chapter 3's expected price gains 355x because Kotlin's
  binomial probability is a sum of logs (no `lgamma` on the JVM), so the old
  O(n²) loop cost more than in C++.
- **`Math.max` was the hidden cost in the American trees.** Kotlin's
  `max(Double, Double)` is `Math.max`, which checks for NaN and signed zero on
  every call. With the price table alone, chapter 7 was only 9.9x faster; with
  `if (v < e) v = e` in the exercise loop (the values are never NaN) it is
  30x, in line with C++. Chapters 7 and 8 both use the plain compare now.
- **`earlyExerciseNodes` (chapter 8) is bound by its output.** At N = 2000 it
  returns about a million `Pair<Int, Int>`, which is most of its 81 MB. It is
  6.3x faster; the rest would need a different return type.
- **Slower on purpose:** `FxMarket` construction (chapter 1) is 0.69x because
  it builds the best-leg table once per market, as in C++. The CRR price table
  in chapter 7 raises allocation from 16 KB to 48 KB at N = 2000.
- **Lattice storage (chapter 11):** rate lattices and `discountBack` results
  are `DoubleArray`s behind `List` views (`asList()`), so the public types are
  unchanged.

## All measurements

### Chapter 1

| Benchmark | Before | After | Speedup | Alloc before | Alloc after |
|---|---:|---:|---:|---:|---:|
| BestCycleScaling.bestCycle/3 | 566 ns | 107 ns | 5.3x | 376 B | 264 B |
| BestCycleScaling.bestCycle/6 | 271 µs | 5.97 µs | 45.4x | 576 B | 304 B |
| BestCycleScaling.bestCycle/8 | 30.8 ms | 256 µs | 120x | 1.52 KB | 744 B |
| Ch01Benchmark.bestCycleBook | 683 ns | 106 ns | 6.4x | 456 B | 296 B |
| Ch01Benchmark.bestLeg | 31.5 ns | 8.26 ns | 3.8x | 0 B | 0 B |
| Ch01Benchmark.fxMarketConstruct | 408 ns | 591 ns | 0.69x | 1.02 KB | 1.37 KB |
| Ch01Benchmark.report | 55.2 µs | 55.4 µs | 1x | 159 KB | 159 KB |

### Chapter 2

| Benchmark | Before | After | Speedup | Alloc before | Alloc after |
|---|---:|---:|---:|---:|---:|
| Ch02Benchmark.report | 26 µs | 25.1 µs | 1x | 71 KB | 69.7 KB |
| CouponBond.couponBondPrice/3 | 62 ns | 35.9 ns | 1.7x | 0 B | 0 B |
| CouponBond.couponBondPrice/30 | 493 ns | 51.2 ns | 9.6x | 0 B | 0 B |
| CouponBond.couponBondPrice/360 | 5.42 µs | 49.8 ns | 109x | 0 B | 0 B |
| CouponBond.yieldToMaturity/3 | 3.05 µs | 169 ns | 18.1x | 0 B | 0 B |
| CouponBond.yieldToMaturity/30 | 22.2 µs | 344 ns | 64.5x | 0 B | 0 B |
| CouponBond.yieldToMaturity/360 | 238 µs | 1.39 µs | 171x | 0 B | 0 B |

### Chapter 3

| Benchmark | Before | After | Speedup | Alloc before | Alloc after |
|---|---:|---:|---:|---:|---:|
| Binomial.binomialProb/3 | 54 ns | 58.1 ns | 0.93x | 0 B | 0 B |
| Binomial.binomialProb/100 | 1.29 µs | 1.02 µs | 1.3x | 0 B | 0 B |
| Binomial.binomialProb/1000 | 11 µs | 11.3 µs | 0.97x | 0 B | 0 B |
| Binomial.expectedPrice/3 | 364 ns | 119 ns | 3.1x | 0 B | 0 B |
| Binomial.expectedPrice/100 | 108 µs | 2.83 µs | 38.1x | 0 B | 0 B |
| Binomial.expectedPrice/1000 | 10.2 ms | 28.7 µs | 355x | 4 B | 0 B |
| Ch03Benchmark.report | 364 µs | 360 µs | 1x | 108 KB | 108 KB |

### Chapter 4

| Benchmark | Before | After | Speedup | Alloc before | Alloc after |
|---|---:|---:|---:|---:|---:|
| Ch04Benchmark.report | 197 µs | 20.9 µs | 9.4x | 100 KB | 65.7 KB |
| ExpectedDiscountedValue.expectedDiscountedValue/4 | 4.94 µs | 462 ns | 10.7x | 17.4 KB | 136 B |
| ExpectedDiscountedValue.expectedDiscountedValue/12 | 5.21 ms | 249 µs | 20.9x | 9.57 MB | 232 B |
| ExpectedDiscountedValue.expectedDiscountedValue/16 | 115 ms | 4.52 ms | 25.5x | 211 MB | 282 B |
| SelfFinancing.selfFinancing/12 | 327 ns | 99.8 ns | 3.3x | 1.14 KB | 288 B |
| SelfFinancing.selfFinancing/1000 | 24.7 µs | 11.4 µs | 2.2x | 76.4 KB | 16.1 KB |

### Chapter 5

| Benchmark | Before | After | Speedup | Alloc before | Alloc after |
|---|---:|---:|---:|---:|---:|
| Assets.minVariancePortfolio/3 | 397 ns | 127 ns | 3.1x | 1.31 KB | 472 B |
| Assets.minVariancePortfolio/50 | 103 µs | 22.9 µs | 4.5x | 175 KB | 23.4 KB |
| Assets.minVariancePortfolio/200 | 3.79 ms | 1.09 ms | 3.5x | 2.57 MB | 333 KB |
| Assets.vecMat/3 | 126 ns | 29.3 ns | 4.3x | 608 B | 56 B |
| Assets.vecMat/50 | 31.5 µs | 2.74 µs | 11.5x | 108 KB | 432 B |
| Assets.vecMat/200 | 784 µs | 47.4 µs | 16.5x | 1.58 MB | 1.63 KB |
| Ch05Benchmark.report | 47.1 µs | 39.5 µs | 1.2x | 137 KB | 124 KB |
| Scenarios.correlation/3 | 72.8 ns | 26.7 ns | 2.7x | 0 B | 0 B |
| Scenarios.correlation/10000 | 105 µs | 40.9 µs | 2.6x | 0 B | 0 B |

### Chapter 7

| Benchmark | Before | After | Speedup | Alloc before | Alloc after |
|---|---:|---:|---:|---:|---:|
| Ch07Benchmark.report | 9.59 ms | 1.28 ms | 7.5x | 187 KB | 299 KB |
| Crr.americanPut/100 | 182 µs | 11.3 µs | 16.1x | 824 B | 2.45 KB |
| Crr.americanPut/2000 | 85.8 ms | 2.82 ms | 30.4x | 16.1 KB | 48 KB |
| Crr.europeanPut/100 | 10.3 µs | 10 µs | 1x | 824 B | 2.45 KB |
| Crr.europeanPut/2000 | 2.62 ms | 2.62 ms | 1x | 16 KB | 48 KB |

### Chapter 8

| Benchmark | Before | After | Speedup | Alloc before | Alloc after |
|---|---:|---:|---:|---:|---:|
| Ch08Benchmark.binomialSum | 38.4 µs | 37.5 µs | 1x | 8.02 KB | 8.02 KB |
| Ch08Benchmark.report | 69 ms | 3.61 ms | 19.1x | 75.1 MB | 223 KB |
| Trees.crrTreePriceAmerican/100 | 261 µs | 16.5 µs | 15.8x | 204 KB | 2.47 KB |
| Trees.crrTreePriceAmerican/2000 | 396 ms | 6.14 ms | 64.4x | 78.2 MB | 48.1 KB |
| Trees.crrTreePriceEuropean/100 | 63.7 µs | 9.96 µs | 6.4x | 204 KB | 2.5 KB |
| Trees.crrTreePriceEuropean/2000 | 102 ms | 2.64 ms | 38.6x | 78.2 MB | 48.1 KB |
| Trees.earlyExerciseNodes/100 | 416 µs | 77 µs | 5.4x | 290 KB | 132 KB |
| Trees.earlyExerciseNodes/2000 | 534 ms | 85.3 ms | 6.3x | 143 MB | 81 MB |
| Trees.optionTreeAmerican/100 | 215 µs | 22 µs | 9.8x | 204 KB | 47 KB |
| Trees.optionTreeAmerican/2000 | 107 ms | 7.72 ms | 13.9x | 78.2 MB | 16.1 MB |

### Chapter 9

| Benchmark | Before | After | Speedup | Alloc before | Alloc after |
|---|---:|---:|---:|---:|---:|
| Ch09Benchmark.bsGreeksCall | 85.1 ns | 68.7 ns | 1.2x | 64 B | 64 B |
| Ch09Benchmark.bsGreeksPut | 90.5 ns | 88.5 ns | 1x | 64 B | 64 B |
| Ch09Benchmark.normInv | 2.19 µs | 33.1 ns | 66.2x | 0 B | 0 B |
| Ch09Benchmark.report | 240 µs | 166 µs | 1.4x | 397 KB | 398 KB |
| Ch09Benchmark.valueAtRisk | 2.41 µs | 170 ns | 14.2x | 0 B | 0 B |

### Chapter 10

| Benchmark | Before | After | Speedup | Alloc before | Alloc after |
|---|---:|---:|---:|---:|---:|
| Ch10Benchmark.report | 117 µs | 98.9 µs | 1.2x | 305 KB | 295 KB |
| Curve.couponBondPrice/3 | 19.9 ns | 21.2 ns | 0.94x | 0 B | 0 B |
| Curve.couponBondPrice/30 | 707 ns | 232 ns | 3.1x | 0 B | 0 B |
| Curve.couponBondPrice/360 | 80.8 µs | 2.66 µs | 30.4x | 0 B | 0 B |
| Curve.yieldToMaturity/3 | 3.97 µs | 126 ns | 31.6x | 0 B | 0 B |
| Curve.yieldToMaturity/30 | 38.6 µs | 1.1 µs | 35x | 0 B | 0 B |
| Curve.yieldToMaturity/360 | 406 µs | 23.4 µs | 17.4x | 0 B | 0 B |
| Hedge.dynamicHedge/5 | 3.63 µs | 1.07 µs | 3.4x | 1.56 KB | 1.56 KB |
| Hedge.dynamicHedge/260 | 193 µs | 52.2 µs | 3.7x | 72.9 KB | 72.9 KB |

### Chapter 11

| Benchmark | Before | After | Speedup | Alloc before | Alloc after |
|---|---:|---:|---:|---:|---:|
| Ch11Benchmark.report | 105 µs | 93.3 µs | 1.1x | 266 KB | 248 KB |
| Curve.bondPrices/10 | 5.46 µs | 516 ns | 10.6x | 17.7 KB | 320 B |
| Curve.bondPrices/50 | 419 µs | 12.1 µs | 34.7x | 975 KB | 1.28 KB |
| Curve.bondPrices/200 | 24 ms | 160 µs | 150x | 51.4 MB | 4.88 KB |
| Lattices.caplet/100 | 128 µs | 41.9 µs | 3x | 195 KB | 816 B |
| Lattices.caplet/1000 | 18.5 ms | 4.58 ms | 4x | 26 MB | 8.02 KB |
| Lattices.rateLattice/100 | 33 µs | 16.1 µs | 2.1x | 149 KB | 44.9 KB |
| Lattices.rateLattice/1000 | 18.2 ms | 1.4 ms | 13x | 14.1 MB | 4.05 MB |
