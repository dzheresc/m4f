# Chapter 4 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`./bench.sh --callgrind` from `ch04/` after a change and compare.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are means of repeated runs; on this
VM they vary by up to about 10%, so compare versions by instruction count (Ir),
which is exact and repeatable.

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_SelfFinancing/12` | 295 ns | 3,990 |
| `BM_SelfFinancing/252` | 3.5 us | 18,613 |
| `BM_SelfFinancing/2520` | 33 us | 235,435 |
| `BM_SelfFinancingMulti/12` (5 stocks, dividends) | 327 ns | 4,006 |
| `BM_SelfFinancingMulti/252` | 5.8 us | 42,133 |
| `BM_SelfFinancingMulti/2520` | 60 us | 416,991 |
| `BM_Gains/12` | 6.8 ns | 132 |
| `BM_Gains/252` | 163 ns | 2,172 |
| `BM_Gains/2520` | 1.5 us | 21,450 |
| `BM_DiscountedGains/12` | 14 ns | 117 |
| `BM_DiscountedGains/252` | 289 ns | 1,917 |
| `BM_DiscountedGains/2520` | 3.2 us | 18,927 |
| `BM_IsArbitrageFree/2`, `/3`, `/100` | 1.0 ns, 2.1 ns, 141 ns | 22, 42, 1,400 |
| `BM_ArbitrageStrategy/2`, `/3`, `/100` | 1.1 ns, 1.3 ns, 73 ns | 22, 29, 708 |
| `BM_RiskNeutralProbs/2`, `/3`, `/100` | 1.2 ns, 23 ns, 439 ns | 25, 373, 3,642 |
| `BM_OneStepValues/2`, `/3`, `/100` | 32 ns, 51 ns, 216 ns | 453, 685, 3,230 |
| `BM_ExpectedDiscountedValue/4` | 8.1 us | 101,338 |
| `BM_ExpectedDiscountedValue/8` | 191 us | 2,245,253 |
| `BM_ExpectedDiscountedValue/12` | 4.4 ms | 44,800,712 |
| `BM_ExpectedDiscountedValue/16` | 88 ms | 853,216,750 |
| `BM_PrintReport` (whole chapter, to memory) | 22 us | 322,747 |

## Observations

- `ExpectedDiscountedValue` enumerates all 2^N paths, which is inherent for an
  arbitrary path-dependent rule. Its cost per path also grows, from about
  6,300 instructions at N = 4 to 13,000 at N = 16. For every path it builds
  fresh `moves`, `s` and `x` vectors, hands the rule a new copy of each
  prefix of `moves` (O(N^2) copying), and `SelfFinancing` copies `s` and `x`
  again into one-row matrices. Walking the tree depth-first would share the
  prefixes between paths and allocate nothing per path.
- `SelfFinancing` costs about 13 ns per step, against 0.6 ns per step for
  `Gains` over the same data. It wraps `s` and `x` in `Mat{...}`, which copies
  both, and grows its outputs with `push_back` without `reserve`.
- `IsArbitrageFree` and `ArbitrageStrategy` scan the returns twice (min and
  max); `std::minmax_element` would do it in one pass. This only matters for
  many scenarios.
- `RiskNeutralProbs` and `OneStepValues` are dominated by allocating their
  result vector when there are few scenarios.

## After: depth-first path walk, allocation-free SelfFinancing

`ExpectedDiscountedValue` walks the binomial tree depth first. Paths share
their prefix, the rule is called once per tree node with the shared `moves`
vector, and nothing is allocated per path. `SelfFinancing` no longer copies
`s` and `x` into one-row matrices (it does the same arithmetic in the same
order), and both strategy functions reserve their outputs.
`IsArbitrageFree` uses `std::minmax_element`. A new test checks the walk
against a direct enumeration of all paths under a real-world p for three
rules, and the report is byte-identical.

| Benchmark | Time before | Time after | Ir before | Ir after | Ir ratio |
|---|---:|---:|---:|---:|---:|
| `BM_ExpectedDiscountedValue/4` | 8.1 us | 0.24 us | 101,338 | 3,216 | 32x |
| `BM_ExpectedDiscountedValue/8` | 191 us | 3.5 us | 2,245,253 | 54,612 | 41x |
| `BM_ExpectedDiscountedValue/16` | 88 ms | 2.8 ms | 853,216,750 | 16,021,212 | 53x |
| `BM_SelfFinancing/12` | 295 ns | 117 ns | 3,990 | 1,054 | 3.8x |
| `BM_SelfFinancing/2520` | 33 us | 21 us | 235,435 | 97,271 | 2.4x |
| `BM_SelfFinancingMulti/2520` | 60 us | 62 us | 416,991 | 371,953 | 1.1x |
| `BM_IsArbitrageFree/100` | 141 ns | 108 ns | 1,400 | 761 | 1.8x |
| `BM_PrintReport` | 22 us | 11 us | 322,747 | 128,953 | 2.5x |
