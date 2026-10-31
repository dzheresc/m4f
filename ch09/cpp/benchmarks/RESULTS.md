# Chapter 9 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`./bench.sh --callgrind` from `ch09/` after a change and compare.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are means of repeated runs; on this
VM they vary by up to about 10%, so compare versions by instruction count (Ir),
which is exact and repeatable.

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_NormCdf` | 12 ns | 128 |
| `BM_NormPdf` | 7.8 ns | 77 |
| `BM_NormInv` | 5.3 us | 27,273 |
| `BM_BsPriceCall` | 47 ns | 395 |
| `BM_BsPricePut` | 50 ns | 401 |
| `BM_BsGreeksCall` | 76 ns | 854 |
| `BM_BsGreeksPut` | 72 ns | 820 |
| `BM_TotalGreek/1`, `/10`, `/100` | 17 ns, 159 ns, 1.7 us | 180, 1,611, 16,284 |
| `BM_DeltaHedge/1`, `/10`, `/100` | 72 ns, 580 ns, 6.1 us | 822, 6,226, 60,836 |
| `BM_DeltaGammaHedge/1`, `/10`, `/100` | 195 ns, 926 ns, 8.2 us | 1,954, 8,900, 79,374 |
| `BM_PortfolioValue/1`, `/10`, `/100` | 41 ns, 388 ns, 3.9 us | 415, 3,983, 39,659 |
| `BM_RebalancePath/52` (10 options) | 37 us | 404,878 |
| `BM_RebalancePath/252` (10 options) | 173 us | 1,915,718 |
| `BM_Quantile` | 5.2 us | 27,224 |
| `BM_Cdf` | 21 ns | 177 |
| `BM_ExpectedCallPayoff` | 37 ns | 338 |
| `BM_ExpectedValue` | 62 ns | 677 |
| `BM_InitialCost` | 59 ns | 627 |
| `BM_ValueAtRisk` | 1.9 us | 25,911 |
| `BM_ExpectedReturn` | 122 ns | 1,374 |
| `BM_PrintReport` (whole chapter, to memory) | 172 us | 1,484,238 |

## Observations

- `NormInv` always runs 200 bisection steps on [-40, 40], each calling
  `NormCdf` (an `exp`), which is about 400 times the cost of `NormCdf`.
  Doubles run out of precision after about 60 halvings, so the last 140 steps
  change nothing. A rational approximation such as Acklam's, refined with one
  Newton or Halley step, is accurate to full double precision at about the
  cost of one `NormCdf`. `Quantile` and `ValueAtRisk` are almost entirely
  `NormInv` (95% of their instructions).
- `BsGreeks` evaluates `NormCdf(d1)` and `NormCdf(d2)` twice each; caching them
  would save about a third. When only one Greek is used (`TotalGreek`), the
  compiler already drops the rest after inlining (180 instructions instead of
  854).
- `RebalancePath` costs about 70 ns per option per step (one delta and one
  price), plus a small `shifted` vector allocated at every step. Fine at these
  sizes.
