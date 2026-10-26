#include "m4f/forwards.hpp"
#include "m4f/futures.hpp"

#include <cmath>
#include <cstdio>

using namespace m4f::ch06;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b, double eps = 1e-9) { return std::abs(a - b) < eps; }

int main() {
    // 6.1.1
    check(near(forwardPrice(100, 0.08, 0.75), 100 * std::exp(0.06)), "F = S e^(rT)");
    check(near(forwardPriceDividends(100, 0.08, 0.75, {}), forwardPrice(100, 0.08, 0.75)), "no dividends");
    check(near(forwardPriceYield(100, 0.08, 0.0, 0.75), forwardPrice(100, 0.08, 0.75)), "zero yield");
    check(near(forwardPriceDividends(100, 0.08, 0.75, {{2, 0.25}, {2, 0.5}}),
               forwardPrice(100, 0.08, 0.75) - 2 * std::exp(0.04) - 2 * std::exp(0.02)),
          "dividends");
    check(near(forwardArbitrageProfit(forwardPrice(100, 0.08, 0.75), 100, 0.08, 0.75), 0), "fair forward");

    // 6.1.2
    const double F0 = forwardPrice(100, 0.08, 0.75);
    check(near(longForwardValue(F0, F0, 0.08, 0.25), 0), "V = 0 at the same price");
    check(near(longForwardValue(F0, forwardPrice(104, 0.08, 0.25), 0.08, 0.25), 104 - F0 * std::exp(-0.08 * 0.25)),
          "long forward = stock - bond");

    // 6.2.1
    const auto tree = futuresTree(100, 0.10, -0.05, 0.03, 3);
    for (int n = 0; n < 4; ++n)
        for (int k = 0; k <= n; ++k)
            check(near(tree[n][k], 100 * std::pow(1.1, k) * std::pow(0.95, n - k) * std::pow(1.03, 3 - n)),
                  "futures = forward with constant r");
    const Vec prices{106.0, 104.5, 101.0, 103.5, 99.0, 102.0};
    const auto days = marginAccount(prices, 100, 1000, 750);
    const double calls[] = {0, 500, 0, 0, 0};
    for (int i = 0; i < 5; ++i) check(near(days[i].call, calls[i]), "margin calls");
    check(near(days.back().balance, 1000 + 500 + 100 * (prices.back() - prices.front())), "final balance");

    // 6.2.2
    const Vec dS{-6.0, -1.0, 2.0, 5.0}, df{-5.0, -2.0, 3.0, 4.0}, p{0.25, 0.25, 0.25, 0.25};
    const double h = minVarianceHedgeRatio(dS, df, p);
    check(near(h, 29.0 / 27), "h*");
    check(hedgedStd(h, dS, df, p) < hedgedStd(h - 0.05, dS, df, p) &&
              hedgedStd(h, dS, df, p) < hedgedStd(h + 0.05, dS, df, p),
          "h* minimises risk");
    check(near(hedgedStd(1.0, dS, dS, p), 0), "perfect hedge");

    if (failures == 0) std::printf("All ch06 tests passed\n");
    return failures == 0 ? 0 : 1;
}
