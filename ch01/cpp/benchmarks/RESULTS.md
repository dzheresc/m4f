# Chapter 1 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`../../bench.sh --callgrind` from this directory (or `./bench.sh --callgrind`
from `ch01/`) after a change and compare.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are the mean of 5 repetitions.
Their coefficient of variation was 1-10% on this VM, so treat differences under
about 10% as noise. Instruction counts (Ir) are exact and repeatable, which
makes them the better number for comparing two versions of the code.

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_Construct` (with precondition `Check`s) | 2.6 ns | 31 |
| `BM_IsArbitrageFree` | 3.8 ns | 22 |
| `BM_Arbitrage` | 3.8 ns | 23 |
| `BM_RiskNeutralProb` | 4.7 ns | 22 |
| `BM_Replicate` (call payoff) | 2.4 ns | 27 |
| `BM_PriceByReplication` (call) | 2.5 ns | 29 |
| `BM_RiskNeutralPrice` (call) | 6.1 ns | 39 |
| `BM_PortfolioRisk` (E and sigma of a 50/50 split) | 7.0 ns | 44 |
| `BM_HedgedPurchaseCost` | 3.7 ns | 39 |
| `BM_ForwardArbitrage` | 2.4 ns | 22 |
| `BM_FxMarketConstruct` (Exercise 1.3 quotes) | 421 ns | 5,517 |
| `BM_BestLeg` | 37.5 ns | 571 |
| `BM_BestCycleBook` (Exercise 1.3, 3 currencies) | 959 ns | 13,263 |
| `BM_BestCycleScaling/3` | 0.84 us | 11,699 |
| `BM_BestCycleScaling/4` | 4.3 us | 58,179 |
| `BM_BestCycleScaling/5` | 31 us | 403,720 |
| `BM_BestCycleScaling/6` | 272 us | 3,272,641 |
| `BM_BestCycleScaling/7` | 2.5 ms | 30,498,718 |
| `BM_BestCycleScaling/8` | 26 ms | 311,063,588 |
| `BM_PrintReport` (whole chapter, to memory) | 25 us | 355,567 |

## Observations

- The one-step market functions cost 2-7 ns (20-45 instructions). They are a
  few multiplications and one or two divisions; the divisions dominate.
  `RiskNeutralPrice` is about twice `Price` because it recomputes `r()`, `u()`
  and `d()` (three divisions) before its own.
- `FxMarket::BestCycle` grows about 10x per extra currency, as expected for an
  exhaustive search over simple cycles. A callgrind profile at 6 currencies puts
  86% of its instructions in `BestLeg` and the `memcmp` of its string
  comparisons: every step of the search rescans all legs and compares currency
  names as strings.
- `PrintReport` is dominated by `std::format`; it is not on any hot path.

## After: best-leg table in `FxMarket`

`FxMarket` now sorts the currencies once and builds an n x n table of best-leg
indices in its constructor; `BestCycle` searches over integer indices and only
copies `FxLeg`s for the winning cycle. Results are unchanged (checked against
the old code on 15,000 random markets with ties). The table makes construction
about twice as expensive, which is paid once per market.

| Benchmark | Time before | Time after | Ir before | Ir after | Ir ratio |
|---|---:|---:|---:|---:|---:|
| `BM_FxMarketConstruct` | 421 ns | 784 ns | 5,517 | 10,096 | 0.55x |
| `BM_BestLeg` | 37.5 ns | 26.7 ns | 571 | 493 | 1.2x |
| `BM_BestCycleBook` | 959 ns | 236 ns | 13,263 | 3,315 | 4.0x |
| `BM_BestCycleScaling/4` | 4.3 us | 0.30 us | 58,179 | 5,319 | 10.9x |
| `BM_BestCycleScaling/6` | 272 us | 4.0 us | 3,272,641 | 68,226 | 48x |
| `BM_BestCycleScaling/8` | 26 ms | 188 us | 311,063,588 | 3,292,582 | 94x |
