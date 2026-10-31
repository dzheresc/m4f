# Chapter 11 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`./bench.sh --callgrind` from `ch11/` after a change and compare. The
callgrind pass takes several minutes because of the 1000-step lattices.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are means of repeated runs; on this
VM they vary by up to about 10%, so compare versions by instruction count (Ir),
which is exact and repeatable.

Monthly short-rate lattices with N steps ((N+1)(N+2)/2 nodes):

| Benchmark | N=10 | N=100 | N=1000 | Ir at N=1000 |
|---|---:|---:|---:|---:|
| `BM_RateLattice` | 759 ns | 20 us | 2.7 ms | 14,897,152 |
| `BM_BondLattice` | 588 ns | 34 us | 4.9 ms | 51,044,891 |
| `BM_ImpliedProbs` | 765 ns | 43 us | 3.7 ms | 48,541,093 |
| `BM_BondOptionEuropean` | 970 ns | 58 us | 5.1 ms | 64,865,436 |
| `BM_BondOptionAmerican` | 980 ns | 52 us | 5.6 ms | 68,558,324 |
| `BM_PayerSwap` | 991 ns | 72 us | 7.2 ms | 97,088,253 |
| `BM_Cap` | 1.1 us | 73 us | 7.9 ms | 98,838,710 |
| `BM_Floor` | 931 ns | 82 us | 8.4 ms | 101,342,710 |
| `BM_Caplet` | 722 ns | 47 us | 4.6 ms | 61,141,331 |

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_BondPrices/10` (zero curve, all maturities) | 3.2 us | 41,532 |
| `BM_BondPrices/50` | 171 us | 2,395,546 |
| `BM_BondPrices/200` | 10 ms | 131,899,524 |
| `BM_MoneyMarket/10`, `/100`, `/1000` | 121 ns, 783 ns, 5.9 us | 1,877, 9,878, 85,337 |
| `BM_SwapRate/10`, `/100` | 3.0 ns, 32 ns | 53, 278 |
| `BM_PrintReport` (whole chapter, to memory) | 33 us | 425,903 |

## Observations

- `BondPrices` runs a separate backward induction for every maturity
  m = 1..N, so the zero curve costs O(N^3): 10 ms at N = 200. Forward
  induction over Arrow-Debreu state prices (each node's price of 1 paid
  there, rolled forward one step at a time) gives every `B(0, m)` in a single
  O(N^2) pass. At N = 200 that is roughly 70x fewer node visits.
- Every lattice function returns the full lattice, allocated row by row.
  `Caplet` builds one to return `[0][0]`. `DiscountBack` fills every row with
  zeros before overwriting it, and `RateLattice` grows each row with
  `push_back`, about 30 instructions per node for one addition.
- The payment and early-exercise callbacks are `std::function`, an indirect
  call per node. `PayerSwap`, `Cap` and `Floor` cost about 1.5x `BondLattice`,
  mostly from the extra `exp` in `FloatingRate` at each node.
- The report uses small lattices, so none of this shows in its output.

## After: forward-induction zero curve, rolling caplet, lattice allocation

- `BondPrices` computes every `B(0, m)` in one forward pass over the
  Arrow-Debreu state prices, O(N^2) instead of one backward induction per
  maturity, O(N^3). It agrees with `BondLattice(m)[0][0]` to 1e-14 relative
  (new test with node-dependent probabilities, N = 60).
- `Caplet` rolls one row back from the fixing level instead of building the
  whole `PricePayments` lattice through a `std::function`; the result is
  bit-identical.
- `RateLattice` sizes each row once instead of growing it with `push_back`,
  and `DiscountBack` no longer zero-fills rows it overwrites.

| Benchmark | Ir before | Ir after | Ir ratio | Time before | Time after |
|---|---:|---:|---:|---:|---:|
| `BM_BondPrices/10` | 41,532 | 7,360 | 5.6x | 3.2 us | 0.55 us |
| `BM_BondPrices/50` | 2,395,546 | 118,655 | 20x | 171 us | 9.8 us |
| `BM_BondPrices/200` | 131,899,524 | 1,752,391 | 75x | 10 ms | 155 us |
| `BM_Caplet/1000` | 61,141,331 | 39,119,175 | 1.6x | 4.6 ms | 2.6 ms |
| `BM_RateLattice/1000` | 14,897,152 | 7,152,194 | 2.1x | 2.7 ms | 1.7 ms |
| `BM_BondLattice/1000` | 51,044,891 | 50,756,669 | 1.0x | 4.9 ms | 3.6 ms |
| `BM_PrintReport` | 425,903 | 404,737 | 1.05x | 33 us | 33 us |

The other lattice functions are dominated by one `exp` per node (the one-step
discount factor, and in swaps and caps a second one in `FloatingRate`), which
this change leaves alone: removing it would mean passing a precomputed
discount lattice through the public API.
