#include "m4f/no_arbitrage.hpp"
#include "m4f/strategies.hpp"

#include <cmath>
#include <cstdio>

using namespace m4f::ch04;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b, double eps = 1e-9) { return std::abs(a - b) < eps; }

int main() {
    // 4.1.1 self-financing
    const Vec S{100.0, 110.0, 104.5, 114.95};
    Vec A;
    for (int n = 0; n < 4; ++n) A.push_back(100.0 * std::pow(1.03, n));
    const Vec x{5.0, 8.0, 3.0};
    const auto [y, V] = selfFinancing(1000.0, S, A, x);
    check(near(V[3], 1079.2915, 1e-4), "V(3)");
    for (int n = 1; n < 3; ++n) check(near(x[n] * S[n] + y[n] * A[n], V[n]), "rebalancing costs V(n)");
    check(near(gains(S, A, x, y), V[3] - V[0]), "gains");
    check(near(discountedGains(S, A, x), V[3] / A[3] - V[0] / A[0]), "discounted gains");

    // 4.2 dividends
    const Mat S2{{50.0, 55.0, 52.0, 58.0}, {20.0, 19.0, 21.0, 22.0}};
    Vec A2;
    for (int n = 0; n < 4; ++n) A2.push_back(std::pow(1.05, n));
    const Mat x2{{10.0, 12.0, 8.0}, {20.0, 15.0, 25.0}};
    const Mat div{{0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 2.0, 0.0}};
    check(near(selfFinancingMulti(1000.0, S2, A2, x2, &div).V[3] - selfFinancingMulti(1000.0, S2, A2, x2).V[3],
               2 * 15 * 1.05),
          "dividend");

    // 4.1.2 arbitrage
    const Vec rets{0.10, -0.05};
    check(isArbitrageFree(rets, 0.03) && !arbitrageStrategy(rets, 0.03, 100, 100), "no arbitrage");
    for (double r : {0.10, 0.12, -0.05, -0.08}) {
        const auto [xs, ys] = *arbitrageStrategy(rets, r, 100, 100);
        check(near(xs * 100 + ys * 100, 0), "V(0) = 0");
        const auto V1 = oneStepValues(xs, ys, rets, r, 100, 100);
        check(std::min(V1[0], V1[1]) >= -1e-12 && std::max(V1[0], V1[1]) > 0, "V(1) >= 0, > 0 somewhere");
    }

    // 4.1.4 FTAP
    for (const Vec& rs : {Vec{0.10, -0.05}, Vec{-0.10, 0.0, 0.10}, Vec{0.3, -0.2, 0.05, 0.1}})
        for (double r : {-0.3, -0.09, 0.0, 0.04, 0.09, 0.5}) {
            const auto q = riskNeutralProbs(rs, r);
            check(q.has_value() == isArbitrageFree(rs, r), "FTAP");
            if (!q) continue;
            double s = 0, e = 0, mn = 1;
            for (std::size_t i = 0; i < rs.size(); ++i) { s += (*q)[i]; e += (*q)[i] * rs[i]; mn = std::min(mn, (*q)[i]); }
            check(near(s, 1) && near(e, r) && mn > 0, "risk-neutral q");
        }
    check(near((*riskNeutralProbs(rets, 0.03))[0], 8.0 / 15), "binomial p*");

    // 4.1.3 martingale property of discounted self-financing values
    const double q = 8.0 / 15;
    const Rule rules[] = {
        [](int, const std::vector<int>& m) { int u = 0; for (int v : m) u += v; return 1.0 + (int(m.size()) - u); },
        [](int, const std::vector<int>& m) { int u = 0; for (int v : m) u += v; return 3.0 * u - 2.0; },
        [](int n, const std::vector<int>&) { return double(n); },
    };
    for (const auto& rule : rules)
        for (double V0 : {0.0, 100.0})
            check(near(expectedDiscountedValue(100, 0.10, -0.05, 0.03, 4, V0, rule, q), V0), "E*(V~(N)) = V(0)");

    if (failures == 0) std::printf("All ch04 tests passed\n");
    return failures == 0 ? 0 : 1;
}
