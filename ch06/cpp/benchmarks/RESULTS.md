# Chapter 6 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`./bench.sh --callgrind` from `ch06/` after a change and compare.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are means of repeated runs; on this
VM they vary by up to about 10%, so compare versions by instruction count (Ir),
which is exact and repeatable.

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_ForwardPrice` | 4.2 ns | 70 |
| `BM_ForwardPriceYield` | 5.1 ns | 71 |
| `BM_ForwardArbitrageProfit` | 5.1 ns | 73 |
| `BM_LongForwardValue` | 4.1 ns | 71 |
| `BM_ForwardPriceDividends/4` | 25 ns | 373 |
| `BM_ForwardPriceDividends/100` | 449 ns | 7,285 |
| `BM_FuturesTree/3` | 233 ns | 3,292 |
| `BM_FuturesTree/10` | 1.0 us | 13,119 |
| `BM_FuturesTree/100` | 26 us | 304,482 |
| `BM_FuturesTree/1000` | 1.1 ms | 13,564,115 |
| `BM_MarginAccount/252` | 1.8 us | 8,129 |
| `BM_MarginAccount/2520` | 17 us | 106,309 |
| `BM_MinVarianceHedgeRatio/3` | 5.0 ns | 105 |
| `BM_MinVarianceHedgeRatio/100` | 136 ns | 1,997 |
| `BM_MinVarianceHedgeRatio/10000` | 13 us | 195,047 |
| `BM_HedgedStd/3` | 52 ns | 785 |
| `BM_HedgedStd/100` | 315 ns | 4,332 |
| `BM_HedgedStd/10000` | 28 us | 362,453 |
| `BM_PrintReport` (whole chapter, to memory) | 10 us | 138,780 |

## Observations

Nothing here is worth changing.

- `FuturesTree` is O(N^2) because it returns every node, at about 27
  instructions per node. Its rows grow by `push_back` without `reserve`, and
  each leaf calls `std::pow` twice; a recurrence would avoid both, but the
  tree is the result, so the order cannot change.
- `MarginAccount` (7 ns per day) and `HedgedStd` (a temporary vector) would
  gain a little from `reserve` or from fusing loops. The gain is small at
  these sizes.
- Everything else is a handful of `exp` calls or one pass over the data.
