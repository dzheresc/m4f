# Chapter 2 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`./bench.sh --callgrind` from `ch02/` after a change and compare.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are means of repeated runs; on this
VM they vary by up to about 10%, so compare versions by instruction count (Ir),
which is exact and repeatable.

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_SimpleValue` | 0.66 ns | 12 |
| `BM_SimpleRate` | 2.5 ns | 16 |
| `BM_PeriodicValue` | 17 ns | 150 |
| `BM_EffectiveRate` | 16 ns | 147 |
| `BM_AnnuityPresentValue` | 17 ns | 158 |
| `BM_AnnuityFutureValue` | 17 ns | 154 |
| `BM_LoanInstallment` | 17 ns | 158 |
| `BM_ContinuousValue` | 4.0 ns | 74 |
| `BM_ContinuousFromPeriodic` | 6.6 ns | 71 |
| `BM_PeriodicFromContinuous` | 5.9 ns | 77 |
| `BM_PeriodicFromPeriodic` | 17 ns | 149 |
| `BM_ZeroCouponPricePeriodic` | 14 ns | 143 |
| `BM_ZeroCouponPriceContinuous` | 4.3 ns | 70 |
| `BM_ImpliedRatePeriodic` | 14 ns | 144 |
| `BM_CouponBondPrice/3` | 46 ns | 578 |
| `BM_CouponBondPrice/10` | 122 ns | 1,572 |
| `BM_CouponBondPrice/30` | 383 ns | 4,412 |
| `BM_CouponBondPrice/60` | 687 ns | 8,672 |
| `BM_CouponBondPrice/360` | 4.7 us | 51,272 |
| `BM_YieldToMaturity/3` | 3.2 us | 26,518 |
| `BM_YieldToMaturity/10` | 6.6 us | 70,562 |
| `BM_YieldToMaturity/30` | 17 us | 196,402 |
| `BM_YieldToMaturity/60` | 33 us | 385,162 |
| `BM_YieldToMaturity/360` | 187 us | 2,274,043 |
| `BM_MoneyMarketRollover/4` | 4.6 ns | 24 |
| `BM_MoneyMarketRollover/120` | 206 ns | 314 |
| `BM_MoneyMarketRollover/1024` | 4.0 us | 2,574 |
| `BM_PrintReport` (whole chapter, to memory) | 9.4 us | 124,224 |

## Observations

- Anything with `std::pow` costs about 15 ns (about 150 instructions); `exp`
  and `log` about 4-7 ns. Those calls are the whole cost of Section 2.1.
- `CouponBondPrice` calls `std::pow` once per coupon plus once for the face
  value, so it is O(n) at about 13 ns per coupon. The discount factors
  `(1+y)^-k` form a geometric series, so the sum has a closed form with one
  `pow` (the annuity factor already in `interest.h`).
- `YieldToMaturity` bisects on [-0.99, 10] down to 1e-12, which takes about 44
  steps, each a full `CouponBondPrice`. That is 3.2 us for the book's 3-coupon
  bond and 187 us for a 30-year monthly bond.
- `MoneyMarketRollover` is a chain of dependent divisions, about 4 ns per step
  at 1024 steps. Not worth changing.
- `PrintReport` is dominated by `std::format`.

## After: closed-form coupon bond price, Newton yield

`CouponBondPrice` now uses the geometric-series closed form (one `log1p`, one
`exp`, one `expm1`), and `YieldToMaturity` uses Newton's method inside the
bisection bracket, starting from the usual yield approximation. The results
agree with a direct sum to 1e-9 relative (new tests cover zero, tiny,
negative and large yields and an out-of-range price), and the report is
byte-identical.

| Benchmark | Time before | Time after | Ir before | Ir after | Ir ratio |
|---|---:|---:|---:|---:|---:|
| `BM_CouponBondPrice/3` | 46 ns | 21 ns | 578 | 189 | 3.1x |
| `BM_CouponBondPrice/30` | 383 ns | 31 ns | 4,412 | 218 | 20x |
| `BM_CouponBondPrice/360` | 4.7 us | 29 ns | 51,272 | 207 | 248x |
| `BM_YieldToMaturity/3` | 3.2 us | 214 ns | 26,518 | 1,001 | 26x |
| `BM_YieldToMaturity/30` | 17 us | 313 ns | 196,402 | 2,310 | 85x |
| `BM_YieldToMaturity/360` | 187 us | 1.7 us | 2,274,043 | 11,937 | 191x |
| `BM_PrintReport` | 9.4 us | 9.4 us | 124,224 | 104,500 | 1.2x |
