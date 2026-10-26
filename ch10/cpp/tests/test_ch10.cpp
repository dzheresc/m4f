#include "m4f/bonds.hpp"
#include "m4f/term_structure.hpp"

#include <cmath>
#include <cstdio>

using namespace m4f::ch10;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b, double eps = 1e-7) { return std::abs(a - b) < eps; }

int main() {
    const std::vector<double> Ts = {0.5, 1.0, 1.5, 2.0, 2.5, 3.0};
    const std::vector<double> Bs = {0.978, 0.953, 0.926, 0.898, 0.870, 0.842};

    const Bond z = zeroCoupon(100.0, 4.0);
    check(near(price(z, 0.06), 100 * std::exp(-0.24)), "zero price");
    check(near(duration(z, 0.06), 4.0) && near(duration(z, 0.06, 1.5), 2.5), "zero duration");
    const Bond b = couponBond(100.0, 5.0, 4);
    check(b.size() == 4 && b[3].t == 4.0 && b[3].C == 105.0 && b[0].C == 5.0, "coupon bond flows");
    const double h = 1e-5;
    check(near((price(b, 0.06 + h) - price(b, 0.06 - h)) / (2 * h), -duration(b, 0.06) * price(b, 0.06), 1e-4), "dP/dy = -DP");
    check(duration(b, 0.06) < 4.0, "D < maturity");

    for (double t : {1.0, 2.5, 4.0}) {
        check(near(investmentValue(b, 0.06, t), price(b, 0.06) * std::exp(0.06 * t)), "unchanged yield grows at y");
        check(near(logReturn(b, 0.06, 0.06, t), 0.06), "log return = y");
    }
    const double D = duration(b, 0.06);
    check(std::abs(investmentValue(b, 0.061, D) - investmentValue(b, 0.06, D)) < 1e-2, "immunised at t=D");
    check(investmentValue(b, 0.07, 1.0) < investmentValue(b, 0.06, 1.0), "short horizon loses when y rises");
    check(investmentValue(b, 0.07, 4.0) > investmentValue(b, 0.06, 4.0), "long horizon gains when y rises");

    const Bond A = couponBond(100.0, 4.0, 5), B = couponBond(100.0, 8.0, 10);
    const auto [w1, w2] = immunisingWeights(duration(A, 0.06), duration(B, 0.06), 5.0);
    check(near(w1 + w2, 1.0) && near(w1 * duration(A, 0.06) + w2 * duration(B, 0.06), 5.0), "immunising weights");
    const Portfolio p = immunise(1000.0, A, B, 0.06, 0.0, 5.0);
    check(near(value(p, A, B, 0.06, 0.0), 1000.0), "immunise invests V");
    const double target = 1000.0 * std::exp(0.06 * 5);
    for (double y : {0.05, 0.07})
        check(p.q1 * investmentValue(A, y, 5.0) + p.q2 * investmentValue(B, y, 5.0) > target - 1e-9, "no shortfall");

    std::vector<std::pair<double, double>> flat;
    for (int i = 0; i <= 10; ++i) flat.push_back({0.5 * i, 0.06});
    const auto steps = dynamicHedge(1000.0, A, B, 5.0, flat);
    check(steps.size() == 11 && near(steps.back().value, 1000.0 * std::exp(0.3), 1e-6), "flat path reaches target");
    for (std::size_t i = 0; i + 1 < steps.size(); ++i) check(near(steps[i].duration, 5.0 - steps[i].t, 1e-9), "D = remaining horizon");
    check(near(dynamicHedge(1000.0, A, B, 5.0, flat, false).back().value, 1000.0 * std::exp(0.3), 1e-6), "static flat path");

    const auto fw = forwardRates(Ts, Bs);
    check(near(fw[0], yieldRate(Bs[0], 0.5)), "first forward = yield");
    const auto back = pricesFromForwardRates(Ts, fw);
    for (std::size_t i = 0; i < Ts.size(); ++i) check(near(back[i], Bs[i]), "prices from forwards");
    check(near(forwardRate(Bs[1], 1.0, Bs[5], 3.0), (fw[2] + fw[3] + fw[4] + fw[5]) / 4), "forward average");
    check(near(impliedPrice(Bs[1], Bs[3]), std::exp(-forwardRate(Bs[1], 1.0, Bs[3], 2.0))), "implied price");
    const auto As = moneyMarketAccount(Ts, Bs);
    for (std::size_t i = 0; i < Ts.size(); ++i) check(near(As[i], 1 / Bs[i]), "A = 1/B");
    const double P = couponBondPrice({1.0, 2.0, 3.0}, {6.0, 6.0, 106.0}, Ts, Bs);
    check(near(P, 6 * Bs[1] + 6 * Bs[3] + 106 * Bs[5]), "coupon bond price");
    const double y = yieldToMaturity(P, {1.0, 2.0, 3.0}, {6.0, 6.0, 106.0});
    check(near(6 * std::exp(-y) + 6 * std::exp(-2 * y) + 106 * std::exp(-3 * y), P, 1e-8), "ytm");
    check(near(yieldToMaturity(Bs[5] * 100, {3.0}, {100.0}), yieldRate(Bs[5], 3.0), 1e-9), "ytm of zero");

    if (failures == 0) std::printf("All C++ tests passed\n");
    return failures == 0 ? 0 : 1;
}
