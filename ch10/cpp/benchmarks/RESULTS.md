# Chapter 10 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`./bench.sh --callgrind` from `ch10/` after a change and compare.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are means of repeated runs; on this
VM they vary by up to about 10%, so compare versions by instruction count (Ir),
which is exact and repeatable.

Bond benchmarks use coupon bonds with n = 3, 10, 60 and 360 cash flows:

| Benchmark | n=3 | n=10 | n=60 | n=360 | Ir at n=360 |
|---|---:|---:|---:|---:|---:|
| `BM_Price` | 17 ns | 53 ns | 279 ns | 1.8 us | 27,370 |
| `BM_Duration` | 37 ns | 113 ns | 581 ns | 4.0 us | 55,456 |
| `BM_LogReturn` | 47 ns | 122 ns | 747 ns | 5.0 us | 57,819 |

Term-structure benchmarks use n = 3, 30 and 360 maturities:

| Benchmark | n=3 | n=30 | n=360 | Ir at n=360 |
|---|---:|---:|---:|---:|
| `BM_ForwardRates` | 84 ns | 380 ns | 3.3 us | 52,641 |
| `BM_PricesFromForwardRates` | 69 ns | 298 ns | 2.1 us | 31,004 |
| `BM_CouponBondPrice` | 6.1 ns | 420 ns | 47 us | 392,058 |
| `BM_YieldToMaturity` | 5.2 us | 32 us | 376 us | 5,755,776 |
| `BM_MoneyMarketAccount` | 48 ns | 222 ns | 2.3 us | 8,734 |

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_Immunise` | 519 ns | 3,363 |
| `BM_DynamicHedge/5` | 1.9 us | 24,297 |
| `BM_DynamicHedge/52` | 22 us | 276,281 |
| `BM_DynamicHedge/260` | 103 us | 1,389,867 |
| `BM_PrintReport` (whole chapter, to memory) | 55 us | 684,803 |

## Observations

- `YieldToMaturity` always runs 200 bisection steps on [-1, 1] with no
  tolerance check, and each step re-sums every cash flow with an `exp`. About
  50 steps already reach full double precision. Newton's method converges in
  a handful of steps here, since the derivative is minus the price times the
  duration, which the same loop can compute. This is the same pattern as
  `YieldToMaturity` in chapter 2 and `NormInv` in chapter 9.
- `CouponBondPrice` matches each cash flow to its maturity by a linear search,
  O(n^2): 47 us for 360 flows. Both lists are sorted, so a single merge pass
  is O(n). The search also relies on exact `==` on doubles, which holds for
  the report's inputs but is fragile.
- `Duration` calls `Price` again for its denominator, which doubles its cost;
  one loop can accumulate both sums. `DynamicHedge` calls `Price` and
  `Duration` several times per step for the same bond and yield.
- The rest is one `exp` or `log` per element and is fine.

## After: Newton yield, merge-pass coupon pricing, one-pass duration

- `YieldToMaturity` uses Newton's method inside the [-1, 1] bisection
  bracket, starting from the yield the bond would have if all flows were paid
  at their mean time. It takes 2 to 7 iterations instead of 200, and agrees
  with the old bisection to 1e-13, including prices whose root lies outside
  the bracket.
- `CouponBondPrice` matches flows to the ascending maturity grid in one merge
  pass, still by exact `==`. An unsorted flow list restarts the scan, so the
  result is the same as the linear search for any input (tested).
- `PriceAndDuration` computes both sums in one loop; `Duration`, `Immunise`
  and `DynamicHedge` use it, and `DynamicHedge` prices each bond once per
  step. All values are bit-identical to before.

| Benchmark | Ir before | Ir after | Ir ratio | Time before | Time after |
|---|---:|---:|---:|---:|---:|
| `BM_YieldToMaturity/3` | 52,344 | 952 | 55x | 5.2 us | 105 ns |
| `BM_YieldToMaturity/30` | 483,696 | 12,347 | 39x | 32 us | 0.83 us |
| `BM_YieldToMaturity/360` | 5,755,776 | 230,282 | 25x | 376 us | 14 us |
| `BM_CouponBondPrice/360` | 392,058 | 10,089 | 39x | 47 us | 0.80 us |
| `BM_Duration/360` | 55,456 | 30,252 | 1.8x | 4.0 us | 1.9 us |
| `BM_Immunise` | 3,363 | 1,030 | 3.3x | 519 ns | 82 ns |
| `BM_DynamicHedge/260` | 1,389,867 | 289,974 | 4.8x | 103 us | 20 us |
| `BM_PrintReport` | 684,803 | 500,339 | 1.4x | 55 us | 40 us |
