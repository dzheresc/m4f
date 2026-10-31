// Unit tests for chapter 5. A tiny self-contained harness keeps the build free
// of third-party dependencies; each check mirrors a Kotlin and Python test.
#include <cmath>
#include <cstddef>
#include <cstdio>

#include "m4f/portfolio.h"
#include "m4f/risk.h"

namespace m4f::ch05 {
namespace {

class Checker {
 public:
  void Expect(bool ok, const char* what) {
    if (!ok) {
      std::printf("FAIL: %s\n", what);
      ++failures_;
    }
  }
  int failures() const { return failures_; }

 private:
  int failures_ = 0;
};

bool Near(double a, double b, double eps = 1e-9) {
  return std::abs(a - b) < eps;
}

Vec Unit(std::size_t i) {
  Vec e(3, 0.0);
  e[i] = 1.0;
  return e;
}

const Vec kMeans{0.10, 0.15, 0.20};

Mat MarketCovariance() {
  return CovarianceMatrix({0.15, 0.25, 0.30},
                          {{1.0, 0.3, 0.1}, {0.3, 1.0, 0.4}, {0.1, 0.4, 1.0}});
}

// Section 5.1: scenarios agree with the two-asset formulas.
void TestScenarios(Checker& t) {
  const Vec p{0.2, 0.5, 0.3};
  const Vec k1{-0.10, 0.10, 0.20};
  const Vec k2{0.20, 0.05, -0.05};
  t.Expect(Near(Mean(k1, p), 0.09), "mean");
  t.Expect(Near(Covariance(k1, k2, p), -0.009), "covariance");
  const double rho = Correlation(k1, k2, p);
  for (double w : {0.0, 0.3, 0.5, 1.2}) {
    Vec k_v;
    for (std::size_t i = 0; i < 3; ++i)
      k_v.push_back(w * k1[i] + (1 - w) * k2[i]);
    t.Expect(
        Near(StdDev(k_v, p), TwoAssetStd(w, StdDev(k1, p), StdDev(k2, p), rho)),
        "two-asset sigma");
    t.Expect(Near(Mean(k_v, p), TwoAssetMean(w, Mean(k1, p), Mean(k2, p))),
             "two-asset mu");
  }
}

// Section 5.2.1: the minimum variance weight is a local minimum.
void TestMinVarianceWeight(Checker& t) {
  for (double rho : {-0.9, -0.5, 0.0, 0.3}) {
    const double w = MinVarianceWeight(0.10, 0.20, rho);
    const double s = TwoAssetStd(w, 0.10, 0.20, rho);
    t.Expect(s < TwoAssetStd(w - 0.01, 0.10, 0.20, rho) &&
                 s < TwoAssetStd(w + 0.01, 0.10, 0.20, rho),
             "min variance");
  }
  t.Expect(Near(TwoAssetStd(2.0 / 3, 0.10, 0.20, -1.0), 0.0, 1e-7),
           "riskless at rho=-1");
}

void TestInverse(Checker& t) {
  const Mat c = MarketCovariance();
  const Mat c_inv = Inverse(c);
  for (std::size_t i = 0; i < 3; ++i) {
    const Vec row = VecMat(Unit(i), c);
    for (std::size_t j = 0; j < 3; ++j) {
      t.Expect(Near(Dot(row, {c_inv[0][j], c_inv[1][j], c_inv[2][j]}),
                    i == j ? 1.0 : 0.0),
               "C C^-1 = I");
    }
  }
}

// Section 5.3.2.
void TestEfficientFrontier(Checker& t) {
  const Mat c = MarketCovariance();
  const Vec w_min = MinVariancePortfolio(c);
  t.Expect(Near(Sum(w_min), 1), "weights sum to 1");
  t.Expect(Near(MinVarianceLine(kMeans, c, PortfolioMean(w_min, kMeans))[0],
                w_min[0]),
           "MVP on the line");
  for (double mu : {0.12, 0.15, 0.25}) {
    const Vec w = MinVarianceLine(kMeans, c, mu);
    t.Expect(Near(Sum(w), 1) && Near(PortfolioMean(w, kMeans), mu),
             "line constraints");
    t.Expect(PortfolioStd(w, c) >= PortfolioStd(w_min, c), "above MVP");
    t.Expect(PortfolioStd(w, c) <
                 PortfolioStd({w[0] + 0.01, w[1] - 0.02, w[2] + 0.01}, c),
             "minimal at mu");
  }
}

// Section 5.4.
void TestCapm(Checker& t) {
  const Mat c = MarketCovariance();
  const Vec w_market = MarketPortfolio(kMeans, c, 0.05);
  const double mu_m = PortfolioMean(w_market, kMeans);
  const double sigma_m = PortfolioStd(w_market, c);
  t.Expect(Near(Sum(w_market), 1) && Near(Beta(w_market, w_market, c), 1),
           "market beta");
  t.Expect(Near(CapitalMarketLine(sigma_m, 0.05, mu_m, sigma_m), mu_m),
           "CML through M");
  t.Expect(Near(PortfolioStd(MinVarianceLine(kMeans, c, mu_m), c), sigma_m),
           "M on frontier");
  for (std::size_t i = 0; i < 3; ++i) {
    t.Expect(Near(SecurityMarketLine(Beta(Unit(i), w_market, c), 0.05, mu_m),
                  kMeans[i]),
             "SML");
  }
}

int RunAllTests() {
  Checker t;
  TestScenarios(t);
  TestMinVarianceWeight(t);
  TestInverse(t);
  TestEfficientFrontier(t);
  TestCapm(t);
  if (t.failures() == 0) std::printf("All ch05 tests passed\n");
  return t.failures() == 0 ? 0 : 1;
}

}  // namespace
}  // namespace m4f::ch05

int main() { return m4f::ch05::RunAllTests(); }
