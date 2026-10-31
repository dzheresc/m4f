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
