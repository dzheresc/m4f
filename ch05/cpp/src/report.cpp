#include "m4f/report.hpp"
#include "m4f/portfolio.hpp"
#include "m4f/risk.hpp"

#include <format>
#include <ostream>

namespace m4f::ch05 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

std::string fl(const Vec& vs) {
    std::string s = "[";
    for (std::size_t i = 0; i < vs.size(); ++i) s += (i ? ", " : "") + f(vs[i]);
    return s + "]";
}

Vec unit(std::size_t i, std::size_t n) {
    Vec e(n, 0.0);
    e[i] = 1.0;
    return e;
}

}  // namespace

void printReport(std::ostream& os) {
    os << "Chapter 5: Portfolio Management (illustrative numbers, book formulas)\n";

    os << "\n== 5.1 Risk: scenarios with p = {0.2, 0.5, 0.3} ==\n";
    const Vec p{0.2, 0.5, 0.3}, K1{-0.10, 0.10, 0.20}, K2{0.20, 0.05, -0.05};
    os << std::format("K1={}: mu={} sigma={}\n", fl(K1), f(mean(K1, p)), f(stdDev(K1, p)));
    os << std::format("K2={}: mu={} sigma={}\n", fl(K2), f(mean(K2, p)), f(stdDev(K2, p)));
    os << std::format("Cov(K1,K2)={} rho={}\n", f(covariance(K1, K2, p)), f(correlation(K1, K2, p)));
    Vec KV;
    for (std::size_t i = 0; i < K1.size(); ++i) KV.push_back(0.5 * K1[i] + 0.5 * K2[i]);
    const double rho = correlation(K1, K2, p);
    os << std::format("Half and half: sigma from scenarios={}, from formula={}\n", f(stdDev(KV, p)),
                      f(twoAssetStd(0.5, stdDev(K1, p), stdDev(K2, p), rho)));

    os << "\n== 5.2.1 Two securities: mu1=0.08 sigma1=0.10, mu2=0.12 sigma2=0.20 ==\n";
    for (double r : {-1.0, -0.5, 0.0, 0.5, 1.0}) {
        const double w = minVarianceWeight(0.10, 0.20, r);
        os << std::format("rho={}: min variance w1={} mu={} sigma={}; w1=0.4: sigma={}\n", f(r), f(w),
                          f(twoAssetMean(w, 0.08, 0.12)), f(twoAssetStd(w, 0.10, 0.20, r)),
                          f(twoAssetStd(0.4, 0.10, 0.20, r)));
    }

    os << "\n== 5.3.1 Several securities: m=(0.10, 0.15, 0.20) sigma=(0.15, 0.25, 0.30) ==\n";
    const Vec m{0.10, 0.15, 0.20};
    const Mat C = covarianceMatrix({0.15, 0.25, 0.30}, {{1.0, 0.3, 0.1}, {0.3, 1.0, 0.4}, {0.1, 0.4, 1.0}});
    os << "rho12=0.3 rho13=0.1 rho23=0.4\n";
    const Vec w{0.4, 0.3, 0.3};
    os << std::format("w={}: mu={} sigma={}\n", fl(w), f(portfolioMean(w, m)), f(portfolioStd(w, C)));

    os << "\n== 5.3.2 Efficient frontier ==\n";
    const Vec wmv = minVariancePortfolio(C);
    os << std::format("Minimum variance portfolio w={} mu={} sigma={}\n", fl(wmv), f(portfolioMean(wmv, m)),
                      f(portfolioStd(wmv, C)));
    for (double mu : {0.15, 0.18, 0.21}) {
        const Vec wl = minVarianceLine(m, C, mu);
        os << std::format("mu_V={}: w={} sigma={}\n", f(mu), fl(wl), f(portfolioStd(wl, C)));
    }

    os << "\n== 5.4.1 Capital market line: R=0.05 ==\n";
    const Vec wM = marketPortfolio(m, C, 0.05);
    const double muM = portfolioMean(wM, m), sM = portfolioStd(wM, C);
    os << std::format("Market portfolio w_M={} mu_M={} sigma_M={} slope={}\n", fl(wM), f(muM), f(sM),
                      f((muM - 0.05) / sM));
    os << std::format("On the CML at sigma=0.15: mu={}\n", f(capitalMarketLine(0.15, 0.05, muM, sM)));

    os << "\n== 5.4.2 Beta factor: beta_V = Cov(K_V, K_M) / sigma_M^2 ==\n";
    Vec betas;
    for (std::size_t i = 0; i < 3; ++i) betas.push_back(beta(unit(i, 3), wM, C));
    os << std::format("beta={}\n", fl(betas));
    double wb = 0.0;
    for (std::size_t i = 0; i < 3; ++i) wb += w[i] * betas[i];
    os << std::format("w={}: beta_V={} sum w_i beta_i={}\n", fl(w), f(beta(w, wM, C)), f(wb));

    os << "\n== 5.4.3 Security market line: mu_V = R + (mu_M - R) beta_V ==\n";
    for (std::size_t i = 0; i < 3; ++i)
        os << std::format("security {}: beta={} SML mu={} actual mu={}\n", i + 1, f(betas[i]),
                          f(securityMarketLine(betas[i], 0.05, muM)), f(m[i]));
}

}  // namespace m4f::ch05
