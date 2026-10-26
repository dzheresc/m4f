#include "m4f/portfolio.hpp"
#include "m4f/risk.hpp"

#include <cmath>
#include <cstdio>

using namespace m4f::ch05;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b, double eps = 1e-9) { return std::abs(a - b) < eps; }
static Vec unit(std::size_t i) { Vec e(3, 0.0); e[i] = 1.0; return e; }

int main() {
    const Vec M{0.10, 0.15, 0.20};
    const Mat C = covarianceMatrix({0.15, 0.25, 0.30}, {{1.0, 0.3, 0.1}, {0.3, 1.0, 0.4}, {0.1, 0.4, 1.0}});

    // 5.1 scenarios agree with the two-asset formulas
    const Vec p{0.2, 0.5, 0.3}, K1{-0.10, 0.10, 0.20}, K2{0.20, 0.05, -0.05};
    check(near(mean(K1, p), 0.09), "mean");
    check(near(covariance(K1, K2, p), -0.009), "covariance");
    const double rho = correlation(K1, K2, p);
    for (double w : {0.0, 0.3, 0.5, 1.2}) {
        Vec KV;
        for (std::size_t i = 0; i < 3; ++i) KV.push_back(w * K1[i] + (1 - w) * K2[i]);
        check(near(stdDev(KV, p), twoAssetStd(w, stdDev(K1, p), stdDev(K2, p), rho)), "two-asset sigma");
        check(near(mean(KV, p), twoAssetMean(w, mean(K1, p), mean(K2, p))), "two-asset mu");
    }

    // 5.2.1 minimum variance weight is a local minimum
    for (double r : {-0.9, -0.5, 0.0, 0.3}) {
        const double w = minVarianceWeight(0.10, 0.20, r), s = twoAssetStd(w, 0.10, 0.20, r);
        check(s < twoAssetStd(w - 0.01, 0.10, 0.20, r) && s < twoAssetStd(w + 0.01, 0.10, 0.20, r), "min variance");
    }
    check(near(twoAssetStd(2.0 / 3, 0.10, 0.20, -1.0), 0.0, 1e-7), "riskless at rho=-1");

    // inverse
    const Mat Ci = inverse(C);
    for (std::size_t i = 0; i < 3; ++i) {
        const Vec row = vecMat(unit(i), C);
        for (std::size_t j = 0; j < 3; ++j)
            check(near(dot(row, {Ci[0][j], Ci[1][j], Ci[2][j]}), i == j ? 1.0 : 0.0), "C C^-1 = I");
    }

    // 5.3.2 frontier
    const Vec wmv = minVariancePortfolio(C);
    check(near(sum(wmv), 1), "weights sum to 1");
    check(near(minVarianceLine(M, C, portfolioMean(wmv, M))[0], wmv[0]), "MVP on the line");
    for (double mu : {0.12, 0.15, 0.25}) {
        const Vec w = minVarianceLine(M, C, mu);
        check(near(sum(w), 1) && near(portfolioMean(w, M), mu), "line constraints");
        check(portfolioStd(w, C) >= portfolioStd(wmv, C), "above MVP");
        check(portfolioStd(w, C) < portfolioStd({w[0] + 0.01, w[1] - 0.02, w[2] + 0.01}, C), "minimal at mu");
    }

    // 5.4 CAPM
    const Vec wM = marketPortfolio(M, C, 0.05);
    const double muM = portfolioMean(wM, M), sM = portfolioStd(wM, C);
    check(near(sum(wM), 1) && near(beta(wM, wM, C), 1), "market beta");
    check(near(capitalMarketLine(sM, 0.05, muM, sM), muM), "CML through M");
    check(near(portfolioStd(minVarianceLine(M, C, muM), C), sM), "M on frontier");
    for (std::size_t i = 0; i < 3; ++i)
        check(near(securityMarketLine(beta(unit(i), wM, C), 0.05, muM), M[i]), "SML");

    if (failures == 0) std::printf("All ch05 tests passed\n");
    return failures == 0 ? 0 : 1;
}
