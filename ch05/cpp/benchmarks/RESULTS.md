# Chapter 5 benchmark results

Baseline for the code as of the Google-style refactor. Re-run with
`./bench.sh --callgrind` from `ch05/` after a change and compare. The full
run takes a few minutes because of the 200-asset cases.

**Machine:** cloud VM, 4 vCPU at 2.1 GHz (shared), GCC 13.3, `-O3` Release,
Google Benchmark 1.9.1, Valgrind 3.22. Times are means of repeated runs; on this
VM they vary by up to about 10%, so compare versions by instruction count (Ir),
which is exact and repeatable.

Portfolio benchmarks run at n = 3, 10, 50 and 200 assets.

| Benchmark | n=3 | n=10 | n=50 | n=200 | Ir at n=200 |
|---|---:|---:|---:|---:|---:|
| `BM_CovarianceMatrix` | 67 ns | 243 ns | 2.3 us | 77 us | 293,731 |
| `BM_VecMat` | 221 ns | 1.0 us | 9.3 us | 112 us | 1,269,144 |
| `BM_Inverse` | 151 ns | 1.2 us | 97 us | 4.7 ms | 65,183,274 |
| `BM_PortfolioMean` | 1.8 ns | 3.1 ns | 16 ns | 98 ns | 926 |
| `BM_PortfolioStd` | 204 ns | 1.0 us | 7.8 us | 131 us | 1,266,011 |
| `BM_MinVariancePortfolio` | 403 ns | 2.4 us | 106 us | 3.8 ms | 66,458,312 |
| `BM_MinVarianceLine` | 670 ns | 3.2 us | 112 us | 4.6 ms | 67,736,906 |
| `BM_MarketPortfolio` | 459 ns | 2.5 us | 102 us | 4.9 ms | 66,468,598 |
| `BM_Beta` | 213 ns | 1.1 us | 9.1 us | 108 us | 1,267,560 |

| Benchmark | Time (mean) | Ir / iteration |
|---|---:|---:|
| `BM_Covariance/3`, `/100`, `/10000` | 5.1 ns, 153 ns, 18 us | 115, 1,813, 175,063 |
| `BM_Correlation/3`, `/100`, `/10000` | 13 ns, 400 ns, 55 us | 297, 5,595, 545,145 |
| `BM_TwoAssetStd` | 2.4 ns | 31 |
| `BM_MinVarianceWeight` | 1.5 ns | 26 |
| `BM_PrintReport` (whole chapter, to memory) | 16 us | 220,540 |

## Observations

- `VecMat` costs about 2.8 n^2 ns, about 32 instructions per multiply-add. For
  each output column it allocates a new vector and copies the column into it,
  reading the matrix column by column. Accumulating row by row
  (`out[j] += v[i] * m[i][j]`) needs no allocation and reads memory in order.
  `PortfolioStd` and `Beta` are almost entirely `VecMat`.
- `Inverse` is Gauss-Jordan on an n x 2n augmented matrix, about 8n^3
  instructions (4.7 ms at n = 200). `MinVariancePortfolio`, `MinVarianceLine`
  and `MarketPortfolio` each invert C only to multiply one or two vectors by
  the inverse. Solving `C x = b` directly, for example with a Cholesky
  factorisation since C is symmetric positive definite, takes about a sixth of
  the arithmetic and is numerically better.
- `Correlation` computes each mean five times (`Covariance` three times, and
  each call recomputes both means); a single pass would be about 3x cheaper.
- `PrintReport` only uses 2- and 3-asset examples, so none of this shows in
  the chapter's output.

## After: row-wise VecMat, Cholesky solves, one-pass Correlation

`VecMat` accumulates row by row without allocating; each output still sums
in the same order, so results are bit-identical. The minimum variance
portfolio, the minimum variance line and the market portfolio solve `C x = b`
with a Cholesky factorisation (`SolveSymmetric`, falling back to the inverse
if C is not positive definite) instead of inverting C. `Inverse` skips the
columns Gauss-Jordan has already cleared (bit-identical), and `Correlation`
makes one pass. New tests check `SolveSymmetric` against the inverse on a
40-asset positive definite matrix and an indefinite one, and the report is
byte-identical.

| Benchmark | Time before | Time after | Ir before | Ir after | Ir ratio |
|---|---:|---:|---:|---:|---:|
| `BM_VecMat/3` | 221 ns | 24 ns | 2,895 | 316 | 9.2x |
| `BM_VecMat/200` | 112 us | 11 us | 1,269,144 | 164,137 | 7.7x |
| `BM_PortfolioStd/200` | 131 us | 13 us | 1,266,011 | 164,806 | 7.7x |
| `BM_Beta/200` | 108 us | 11 us | 1,267,560 | 165,751 | 7.6x |
| `BM_Inverse/200` | 4.7 ms | 4.0 ms | 65,183,274 | 49,599,300 | 1.3x |
| `BM_MinVariancePortfolio/3` | 403 ns | 127 ns | 5,665 | 1,227 | 4.6x |
| `BM_MinVariancePortfolio/200` | 3.8 ms | 0.64 ms | 66,458,312 | 6,977,037 | 9.5x |
| `BM_MinVarianceLine/200` | 4.6 ms | 0.64 ms | 67,736,906 | 7,204,080 | 9.4x |
| `BM_MarketPortfolio/200` | 4.9 ms | 0.63 ms | 66,468,598 | 6,981,313 | 9.5x |
| `BM_Correlation/10000` | 55 us | 22 us | 545,145 | 250,076 | 2.2x |
| `BM_PrintReport` | 16 us | 13 us | 220,540 | 167,573 | 1.3x |
