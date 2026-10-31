# Chapter 5: Rust vs C++ benchmark results

Both languages run the same cases (`benches/cases.rs` mirrors
`cpp/benchmarks/*_benchmark.cc`) on the same machine, back to back. Re-run with
`./bench.sh --compare` from `ch05/`.

**Machine:** cloud VM, 4 vCPU Intel Xeon at 2.1 GHz (shared). C++: GCC 13.3,
`-O3` Release, Google Benchmark 1.9.1, median of 5 repetitions. Rust: rustc
1.97, release profile (`opt-level = 3`, one codegen unit, fat LTO), Criterion
0.8, median of 100 samples. Instruction counts (Ir per iteration) come from
Valgrind 3.22 callgrind for both, as (Ir(N2) − Ir(N1)) / (N2 − N1), so set-up
cancels. Ratios below 1 mean Rust is faster or runs fewer instructions.

Wall-clock times on this VM vary by up to 10% run to run; the Ir counts are
exact. Random inputs are drawn from the same ranges in both languages but by
different generators (`std::mt19937_64` in C++, SplitMix64 in Rust).

| Benchmark | C++ time | Rust time | Rust/C++ | C++ Ir | Rust Ir | Rust/C++ |
|---|---:|---:|---:|---:|---:|---:|
| `Covariance/3` | 5.68 ns | 5.27 ns | 0.93 | 115 | 113 | 0.98 |
| `Covariance/100` | 150 ns | 126 ns | 0.84 | 1,813 | 1,648 | 0.91 |
| `Covariance/10000` | 17.2 µs | 21.7 µs | 1.26 | 175,063 | 160,048 | 0.91 |
| `Correlation/3` | 7.98 ns | 7.13 ns | 0.89 | 151 | 161 | 1.07 |
| `Correlation/100` | 188 ns | 199 ns | 1.06 | 2,576 | 2,718 | 1.06 |
| `Correlation/10000` | 21.4 µs | 24.2 µs | 1.13 | 250,076 | 265,068 | 1.06 |
| `TwoAssetStd` | 2.12 ns | 2.16 ns | 1.02 | 31 | 32 | 1.03 |
| `MinVarianceWeight` | 1.79 ns | 1.79 ns | 1.00 | 26 | 26 | 1.00 |
| `CovarianceMatrix/3` | 77 ns | 42.7 ns | 0.55 | 1,142 | 766 | 0.67 |
| `CovarianceMatrix/10` | 213 ns | 181 ns | 0.85 | 3,452 | 2,970 | 0.86 |
| `CovarianceMatrix/50` | 1.87 µs | 2.92 µs | 1.56 | 36,739 | 44,721 | 1.22 |
| `CovarianceMatrix/200` | 70 µs | 58.1 µs | 0.83 | 293,532 | 484,013 | 1.65 |
| `VecMat/3` | 21.2 ns | 28.2 ns | 1.33 | 316 | 430 | 1.36 |
| `VecMat/10` | 57.1 ns | 62.1 ns | 1.09 | 810 | 1,034 | 1.28 |
| `VecMat/50` | 518 ns | 657 ns | 1.27 | 11,114 | 10,490 | 0.94 |
| `VecMat/200` | 11.3 µs | 13.1 µs | 1.16 | 164,137 | 136,118 | 0.83 |
| `Inverse/3` | 156 ns | 170 ns | 1.08 | 2,646 | 2,901 | 1.10 |
| `Inverse/10` | 1.22 µs | 1.4 µs | 1.15 | 16,848 | 21,546 | 1.28 |
| `Inverse/50` | 70.2 µs | 73.9 µs | 1.05 | 895,743 | 879,702 | 0.98 |
| `Inverse/200` | 3.72 ms | 4.28 ms | 1.15 | 49,599,300 | 42,732,425 | 0.86 |
| `PortfolioMean/3` | 1.97 ns | 2.05 ns | 1.04 | 40 | 45 | 1.12 |
| `PortfolioMean/10` | 3.15 ns | 4.21 ns | 1.33 | 71 | 72 | 1.01 |
| `PortfolioMean/50` | 17.4 ns | 16.7 ns | 0.96 | 251 | 222 | 0.88 |
| `PortfolioMean/200` | 96.7 ns | 77.8 ns | 0.81 | 926 | 775 | 0.84 |
| `PortfolioStd/3` | 33.3 ns | 23.8 ns | 0.72 | 333 | 471 | 1.41 |
| `PortfolioStd/10` | 54.9 ns | 74.6 ns | 1.36 | 852 | 1,102 | 1.29 |
| `PortfolioStd/50` | 597 ns | 568 ns | 0.95 | 11,296 | 10,708 | 0.95 |
| `PortfolioStd/200` | 9.61 µs | 10.1 µs | 1.05 | 164,806 | 136,888 | 0.83 |
| `MinVariancePortfolio/3` | 125 ns | 77.6 ns | 0.62 | 1,227 | 1,377 | 1.12 |
| `MinVariancePortfolio/10` | 698 ns | 498 ns | 0.71 | 3,823 | 5,920 | 1.55 |
| `MinVariancePortfolio/50` | 14.6 µs | 12.2 µs | 0.84 | 157,670 | 188,646 | 1.20 |
| `MinVariancePortfolio/200` | 656 µs | 610 µs | 0.93 | 6,977,031 | 6,658,133 | 0.95 |
| `MinVarianceLine/3` | 248 ns | 122 ns | 0.49 | 2,568 | 2,076 | 0.81 |
| `MinVarianceLine/10` | 1.03 µs | 688 ns | 0.67 | 6,552 | 7,896 | 1.21 |
| `MinVarianceLine/50` | 16.8 µs | 14.1 µs | 0.84 | 176,129 | 212,635 | 1.21 |
| `MinVarianceLine/200` | 617 µs | 592 µs | 0.96 | 7,204,078 | 6,989,709 | 0.97 |
| `MarketPortfolio/3` | 149 ns | 85.6 ns | 0.57 | 1,683 | 1,383 | 0.82 |
| `MarketPortfolio/10` | 745 ns | 495 ns | 0.66 | 4,788 | 5,929 | 1.24 |
| `MarketPortfolio/50` | 14.1 µs | 13.3 µs | 0.95 | 159,559 | 188,695 | 1.18 |
| `MarketPortfolio/200` | 663 µs | 548 µs | 0.83 | 6,981,305 | 6,666,314 | 0.95 |
| `Beta/3` | 29.9 ns | 27 ns | 0.90 | 359 | 523 | 1.46 |
| `Beta/10` | 61.5 ns | 63.1 ns | 1.03 | 904 | 1,182 | 1.31 |
| `Beta/50` | 792 ns | 669 ns | 0.84 | 11,528 | 10,938 | 0.95 |
| `Beta/200` | 10.5 µs | 10.5 µs | 1.01 | 165,751 | 137,674 | 0.83 |
| `PrintReport` | 12.6 µs | 15.3 µs | 1.21 | 167,573 | 228,165 | 1.36 |

## Observations

- **The portfolio solvers (`MinVariancePortfolio`, `MinVarianceLine`,
  `MarketPortfolio`) are 0.83–0.96x the C++ time at 200 assets.** All three
  go through `solve_symmetric`, a Cholesky solve. The first port indexed
  `l[i * n + k]` in the inner dot products and ran 2.1–2.5x the C++
  instructions, because every index was bounds-checked. Taking the two row
  prefixes as slices and zipping them (and walking the transposed column
  with `step_by(n)`) brought them to 0.95–0.97x. Between 10 and 50 assets
  they still run 1.2–1.55x the instructions but less time.
- **`Covariance`** dropped from 1.23x to 0.91x the instructions once its
  loop zipped the three slices instead of indexing them. Its row was re-run
  after the change; the other rows are from the full run before it.
- **`Inverse`** (Gauss-Jordan with partial pivoting on `Vec<Vec<f64>>`) runs
  0.86x the instructions at 200 assets and is within ±15% in time.
- **`CovarianceMatrix`** is 0.55x the time at 3 assets, where the C++
  allocates each row separately and Rust collects them in place. At 50–200
  assets Rust runs more instructions (1.2–1.65x) in about the same or less
  time; LLVM unrolls the inner product differently.
- **Small vectors (3–10 assets)** cost a fixed 20–150 instructions more in
  `VecMat`, `PortfolioStd` and `Beta`: bounds checks and the length checks
  `zip` adds. From 50 assets on the same functions run 0.83–0.95x.
- **`PrintReport`** is 1.36x the instructions and 1.2x the time, as in the
  other chapters: Rust's `fmt` costs more than `std::format` per number.
