// Microbenchmarks for the n-asset portfolio formulas (m4f/portfolio.h) at 3
// assets (the book's examples), 10, 50 and 200.
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/portfolio.h"

namespace m4f::ch05 {
namespace {

// A reproducible market of n assets: expected returns in [2%, 15%] and a
// positive definite covariance matrix from a two-factor model plus
// idiosyncratic variance.
struct Market {
  Vec m;
  Vec sigma;
  Mat rho;
  Mat cov;
  Vec w;  // Some fully invested portfolio.
};

Market MakeMarket(std::size_t n) {
  std::mt19937_64 rng(n);
  std::uniform_real_distribution<double> mean(0.02, 0.15);
  std::uniform_real_distribution<double> loading(-0.2, 0.3);
  std::uniform_real_distribution<double> idio(0.01, 0.04);
  Mat factors(n, Vec(2));
  Market mk;
  for (std::size_t i = 0; i < n; ++i) {
    mk.m.push_back(mean(rng));
    factors[i] = {loading(rng), loading(rng)};
  }
  mk.cov.assign(n, Vec(n));
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      mk.cov[i][j] =
          factors[i][0] * factors[j][0] + factors[i][1] * factors[j][1];
    }
    mk.cov[i][i] += idio(rng);
  }
  for (std::size_t i = 0; i < n; ++i) {
    mk.sigma.push_back(std::sqrt(mk.cov[i][i]));
  }
  mk.rho.assign(n, Vec(n));
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      mk.rho[i][j] = mk.cov[i][j] / (mk.sigma[i] * mk.sigma[j]);
    }
  }
  mk.w.assign(n, 1.0 / static_cast<double>(n));
  return mk;
}

void AssetCounts(benchmark::internal::Benchmark* b) {
  for (int n : {3, 10, 50, 200}) b->Arg(n);
  b->Complexity();
}

#define M4F_MARKET_BENCHMARK(name, expr)                                    \
  void name(benchmark::State& state) {                                      \
    const Market mk = MakeMarket(static_cast<std::size_t>(state.range(0))); \
    for (auto _ : state) benchmark::DoNotOptimize(expr);                    \
    state.SetComplexityN(state.range(0));                                   \
  }                                                                         \
  BENCHMARK(name)->Apply(AssetCounts)

M4F_MARKET_BENCHMARK(BM_CovarianceMatrix, CovarianceMatrix(mk.sigma, mk.rho));
M4F_MARKET_BENCHMARK(BM_VecMat, VecMat(mk.w, mk.cov));
M4F_MARKET_BENCHMARK(BM_Inverse, Inverse(mk.cov));
M4F_MARKET_BENCHMARK(BM_PortfolioMean, PortfolioMean(mk.w, mk.m));
M4F_MARKET_BENCHMARK(BM_PortfolioStd, PortfolioStd(mk.w, mk.cov));
M4F_MARKET_BENCHMARK(BM_MinVariancePortfolio, MinVariancePortfolio(mk.cov));
M4F_MARKET_BENCHMARK(BM_MinVarianceLine, MinVarianceLine(mk.m, mk.cov, 0.1));
M4F_MARKET_BENCHMARK(BM_MarketPortfolio, MarketPortfolio(mk.m, mk.cov, 0.01));
M4F_MARKET_BENCHMARK(BM_Beta, Beta(mk.w, mk.m, mk.cov));

#undef M4F_MARKET_BENCHMARK

}  // namespace
}  // namespace m4f::ch05
