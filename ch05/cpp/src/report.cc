#include "m4f/report.h"

#include <cstddef>
#include <format>
#include <ostream>
#include <string>

#include "m4f/portfolio.h"
#include "m4f/risk.h"

namespace m4f::ch05 {
namespace {

// Four decimals, round-half-even on the exact double. Adding 0.0 turns -0.0
// into 0.0 so that no "-0.0000" appears.
std::string Fmt(double v) { return std::format("{:.4f}", v + 0.0); }

// "[a, b, c]" with each entry formatted by Fmt().
std::string FmtList(const Vec& values) {
  std::string out = "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    out += (i ? ", " : "") + Fmt(values[i]);
  }
  return out + "]";
}

// The i-th unit vector of length n.
Vec Unit(std::size_t i, std::size_t n) {
  Vec e(n, 0.0);
  e[i] = 1.0;
  return e;
}

// The three-security market shared by sections 5.3 and 5.4.
const Vec kMeans{0.10, 0.15, 0.20};
const Vec kWeights{0.4, 0.3, 0.3};
constexpr double kRiskFree = 0.05;

Mat MarketCovariance() {
  return CovarianceMatrix({0.15, 0.25, 0.30},
                          {{1.0, 0.3, 0.1}, {0.3, 1.0, 0.4}, {0.1, 0.4, 1.0}});
}

void PrintRisk(std::ostream& os) {
  os << "\n== 5.1 Risk: scenarios with p = {0.2, 0.5, 0.3} ==\n";
  const Vec p{0.2, 0.5, 0.3};
  const Vec k1{-0.10, 0.10, 0.20};
  const Vec k2{0.20, 0.05, -0.05};
  os << std::format("K1={}: mu={} sigma={}\n", FmtList(k1), Fmt(Mean(k1, p)),
                    Fmt(StdDev(k1, p)));
  os << std::format("K2={}: mu={} sigma={}\n", FmtList(k2), Fmt(Mean(k2, p)),
                    Fmt(StdDev(k2, p)));
  os << std::format("Cov(K1,K2)={} rho={}\n", Fmt(Covariance(k1, k2, p)),
                    Fmt(Correlation(k1, k2, p)));
  Vec k_v;
  for (std::size_t i = 0; i < k1.size(); ++i) {
    k_v.push_back(0.5 * k1[i] + 0.5 * k2[i]);
  }
  const double rho = Correlation(k1, k2, p);
  os << std::format("Half and half: sigma from scenarios={}, from formula={}\n",
                    Fmt(StdDev(k_v, p)),
                    Fmt(TwoAssetStd(0.5, StdDev(k1, p), StdDev(k2, p), rho)));
}

void PrintTwoSecurities(std::ostream& os) {
  os << "\n== 5.2.1 Two securities: mu1=0.08 sigma1=0.10, mu2=0.12 "
        "sigma2=0.20 ==\n";
  for (double rho : {-1.0, -0.5, 0.0, 0.5, 1.0}) {
    const double w = MinVarianceWeight(0.10, 0.20, rho);
    os << std::format(
        "rho={}: min variance w1={} mu={} sigma={}; w1=0.4: sigma={}\n",
        Fmt(rho), Fmt(w), Fmt(TwoAssetMean(w, 0.08, 0.12)),
        Fmt(TwoAssetStd(w, 0.10, 0.20, rho)),
        Fmt(TwoAssetStd(0.4, 0.10, 0.20, rho)));
  }
}

void PrintSeveralSecurities(std::ostream& os, const Mat& c) {
  os << "\n== 5.3.1 Several securities: m=(0.10, 0.15, 0.20) "
        "sigma=(0.15, 0.25, 0.30) ==\n";
  os << "rho12=0.3 rho13=0.1 rho23=0.4\n";
  os << std::format("w={}: mu={} sigma={}\n", FmtList(kWeights),
                    Fmt(PortfolioMean(kWeights, kMeans)),
                    Fmt(PortfolioStd(kWeights, c)));
}

void PrintEfficientFrontier(std::ostream& os, const Mat& c) {
  os << "\n== 5.3.2 Efficient frontier ==\n";
  const Vec w_min = MinVariancePortfolio(c);
  os << std::format("Minimum variance portfolio w={} mu={} sigma={}\n",
                    FmtList(w_min), Fmt(PortfolioMean(w_min, kMeans)),
                    Fmt(PortfolioStd(w_min, c)));
  for (double mu : {0.15, 0.18, 0.21}) {
    const Vec w = MinVarianceLine(kMeans, c, mu);
    os << std::format("mu_V={}: w={} sigma={}\n", Fmt(mu), FmtList(w),
                      Fmt(PortfolioStd(w, c)));
  }
}

void PrintCapm(std::ostream& os, const Mat& c) {
  os << "\n== 5.4.1 Capital market line: R=0.05 ==\n";
  const Vec w_market = MarketPortfolio(kMeans, c, kRiskFree);
  const double mu_m = PortfolioMean(w_market, kMeans);
  const double sigma_m = PortfolioStd(w_market, c);
  os << std::format("Market portfolio w_M={} mu_M={} sigma_M={} slope={}\n",
                    FmtList(w_market), Fmt(mu_m), Fmt(sigma_m),
                    Fmt((mu_m - kRiskFree) / sigma_m));
  os << std::format("On the CML at sigma=0.15: mu={}\n",
                    Fmt(CapitalMarketLine(0.15, kRiskFree, mu_m, sigma_m)));

  os << "\n== 5.4.2 Beta factor: beta_V = Cov(K_V, K_M) / sigma_M^2 ==\n";
  Vec betas;
  for (std::size_t i = 0; i < 3; ++i) {
    betas.push_back(Beta(Unit(i, 3), w_market, c));
  }
  os << std::format("beta={}\n", FmtList(betas));
  double weighted_beta = 0.0;
  for (std::size_t i = 0; i < 3; ++i) weighted_beta += kWeights[i] * betas[i];
  os << std::format("w={}: beta_V={} sum w_i beta_i={}\n", FmtList(kWeights),
                    Fmt(Beta(kWeights, w_market, c)), Fmt(weighted_beta));

  os << "\n== 5.4.3 Security market line: mu_V = R + (mu_M - R) beta_V ==\n";
  for (std::size_t i = 0; i < 3; ++i) {
    os << std::format(
        "security {}: beta={} SML mu={} actual mu={}\n", i + 1, Fmt(betas[i]),
        Fmt(SecurityMarketLine(betas[i], kRiskFree, mu_m)), Fmt(kMeans[i]));
  }
}

}  // namespace

void PrintReport(std::ostream& os) {
  os << "Chapter 5: Portfolio Management (illustrative numbers, book "
        "formulas)\n";
  const Mat c = MarketCovariance();
  PrintRisk(os);
  PrintTwoSecurities(os);
  PrintSeveralSecurities(os, c);
  PrintEfficientFrontier(os, c);
  PrintCapm(os, c);
}

}  // namespace m4f::ch05
