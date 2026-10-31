# Chapter 3 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`./bench.sh --callgrind` from `ch03/` after a change and compare.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are means of repeated runs; on this
VM they vary by up to about 10%, so compare versions by instruction count (Ir),
which is exact and repeatable.

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_BinomialProb/3` | 30 ns | 312 |
| `BM_BinomialProb/10` | 31 ns | 356 |
| `BM_BinomialProb/100` | 198 ns | 851 |
| `BM_BinomialProb/1000` | 2.5 us | 5,817 |
| `BM_Price` | 26 ns | 289 |
| `BM_RiskNeutralProb` | 1.2 ns | 14 |
| `BM_ExpectedPrice/3` | 154 ns | 1,946 |
| `BM_ExpectedPrice/10` | 554 ns | 6,489 |
| `BM_ExpectedPrice/100` | 29 us | 112,914 |
| `BM_ExpectedPrice/1000` | 2.7 ms | 6,094,545 |
| `BM_ConditionalDiscounted` | 79 ns | 863 |
| `BM_TrinomialRiskNeutralProbs` | 1.3 ns | 21 |
| `BM_NormCdf` | 7.4 ns | 99 |
| `BM_BinomialExpectedPrice/10` .. `/10000` | 25-27 ns | 278 |
| `BM_LognormalExpectedPrice` | 5.0 ns | 72 |
| `BM_BinomialProbAbove/10` | 124 ns | 1,673 |
| `BM_BinomialProbAbove/100` | 1.2 us | 16,943 |
| `BM_BinomialProbAbove/1000` | 13 us | 170,752 |
| `BM_BinomialProbAbove/10000` | 125 us | 1,730,448 |
| `BM_LognormalProbAbove` | 9.2 ns | 101 |
| `BM_Returns/252` | 435 ns | 6,385 |
| `BM_Returns/2520` | 3.8 us | 60,624 |
| `BM_LogReturns/252` | 1.5 us | 18,962 |
| `BM_LogReturns/2520` | 12 us | 186,621 |
| `BM_CompoundReturn/252` | 246 ns | 1,524 |
| `BM_CompoundReturn/2520` | 3.2 us | 15,132 |
| `BM_ExpectationVariance/252` | 435 ns | 3,720 |
| `BM_ExpectationVariance/2520` | 4.6 us | 36,606 |
| `BM_PrintReport` (whole chapter, to memory) | 148 us | 2,117,631 |

## Observations

- `BinomialTree::ExpectedPrice` is O(n^2): it calls `BinomialProb` for each of
  the n+1 nodes, and `BinomialProb` builds the binomial coefficient in a loop
  of k steps. It takes 2.7 ms at n = 1000. The answer has the closed form
  `S(0) (1 + p u + (1-p) d)^n`, and an O(n) sum is also easy.
- `BinomialProb` overflows for large n. The coefficient is built in full
  before multiplying by `p^k (1-p)^(n-k)`, so `C(n, n/2)` passes `DBL_MAX`
  near n = 1030. `ExpectedPrice(1030, ...)` returns `inf`, and at n = 2000 it
  returns `nan`. The report only uses n <= 3, so its output is unaffected.
  The benchmarks stop at n = 1000 for this reason.
- `BinomialProbAbove` is O(n) at about 12 ns per step, mostly two `std::log`
  calls per step to update the log binomial coefficient. At N = 10000 it
  accounts for most of `PrintReport`'s 148 us.
- `Returns` and `LogReturns` do not reserve their output vector; at these sizes
  the reallocations are a small share next to the `log` calls.
- `BinomialExpectedPrice` is O(1) in N (a single `pow`), as it should be.

## After: log-space binomial probabilities, O(n) expected price

`BinomialProb` now works in log space (`lgamma` in C++ and Python, a sum of
logs in Kotlin), which fixes the overflow in all three languages; new tests
check that the probabilities for n = 2000 sum to 1 and that
`C(2000,1000)/2^2000` is right. `ExpectedPrice` (C++) sums the n+1 node terms
in O(n), updating `log C(n,k)` from term to term, and matches the closed form
`S(0) (1 + p u + (1-p) d)^n` at n = 2000. The report is byte-identical in all
three languages.

| Benchmark | Time before | Time after | Ir before | Ir after | Ir ratio |
|---|---:|---:|---:|---:|---:|
| `BM_BinomialProb/3` | 30 ns | 58 ns | 312 | 654 | 0.48x |
| `BM_BinomialProb/1000` | 2.5 us | 62 ns | 5,817 | 691 | 8.4x |
| `BM_ExpectedPrice/3` | 154 ns | 59 ns | 1,946 | 754 | 2.6x |
| `BM_ExpectedPrice/100` | 29 us | 1.6 us | 112,914 | 20,639 | 5.5x |
| `BM_ExpectedPrice/1000` | 2.7 ms | 16 us | 6,094,545 | 209,782 | 29x |

`BinomialProb` for small n is now about twice as expensive (three `lgamma`
calls instead of a short loop), in exchange for constant cost and correct
results at any n. `ExpectedPrice(1030, ...)` used to return `inf`; it now
returns the right value.
