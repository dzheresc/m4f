# Chapter 7 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`./bench.sh --callgrind` from `ch07/` after a change and compare. The
callgrind pass takes a few minutes because of the 2000-step American cases.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are means of repeated runs; on this
VM they vary by up to about 10%, so compare versions by instruction count (Ir),
which is exact and repeatable.

CRR binomial pricing, at the money, one year, r = 5%, sigma = 20%:

| Benchmark | N=10 | N=100 | N=500 | N=2000 | Ir at N=2000 |
|---|---:|---:|---:|---:|---:|
| `BM_CrrEuropeanCall` | 0.43 us | 5.7 us | 93 us | 1.3 ms | 10,634,144 |
| `BM_CrrEuropeanPut` | 0.40 us | 5.9 us | 92 us | 1.8 ms | 10,637,418 |
| `BM_CrrAmericanCall` | 1.6 us | 130 us | 2.8 ms | 49 ms | 596,777,025 |
| `BM_CrrAmericanPut` | 1.5 us | 128 us | 3.2 ms | 50 ms | 602,449,553 |

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_CallPayoff` | 0.81 ns | 11 |
| `BM_Profit` | 0.90 ns | 10 |
| `BM_ParityGap` | 0.61 ns | 12 |
| `BM_EuropeanBounds` | 1.7 ns | 19 |
| `BM_AmericanParityBounds` | 0.58 ns | 14 |
| `BM_ButterflyGap` | 1.4 ns | 21 |
| `BM_PrintReport` (whole chapter, to memory) | 7.5 ms | 82,435,899 |

(The `exp` in the scalar formulas is hoisted out of the loop because r and T
are fixed there; these numbers show the arithmetic only.)

## Observations

- American pricing in `CrrPrice` is about 37x slower than European, 300
  instructions per node against 5. For the early-exercise check it recomputes
  the node's stock price with two `std::pow` calls at every node of the
  O(N^2) backward induction. Since `down = 1/up`, the price at node (n, k) is
  `s0 * up^(2k-n)`, which can be read from a table of 2N+1 powers built once,
  or updated by multiplying by `up^2` along each row. That makes American
  pricing cost about the same as European.
- `PrintReport` spends almost all of its 7.5 ms in American prices at 200
  steps, so the change above would also make the report about 30x faster.
- The leaves are also built with two `pow` calls each, which is O(N) and
  does not matter.
