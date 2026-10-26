#include "m4f/binomial.hpp"
#include "m4f/options.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace m4f::ch07;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b, double eps = 1e-9) { return std::abs(a - b) < eps; }
static double price(double S0, double X, double T, bool isCall, bool american = false) {
    return crrPrice(S0, X, 0.05, 0.2, T, 200, isCall, american);
}

int main() {
    for (double S : {80.0, 100.0, 120.0}) check(near(callPayoff(S, 100) - putPayoff(S, 100), S - 100), "payoffs");
    check(near(profit(0.0, 10.0, 0.05, 1.0), -10 * std::exp(0.05)), "profit");

    check(near(parityGap(price(100, 100, 1, true), price(100, 100, 1, false), 100, 100, 0.05, 1), 0), "parity");
    check(near(parityGap(12.0, 5.0, 100, 100, 0.05, 1), 7 - 100 + 100 * std::exp(-0.05)), "parity gap");

    for (double S0 : {80.0, 100.0, 120.0})
        for (double T : {0.25, 1.0}) {
            check(boundViolation(price(S0, 100, T, true), europeanCallBounds(S0, 100, 0.05, T)) == 0, "call bounds");
            check(boundViolation(price(S0, 100, T, false), europeanPutBounds(S0, 100, 0.05, T)) == 0, "put bounds");
            const Bounds b = americanParityBounds(S0, 100, 0.05, T);
            const double d = price(S0, 100, T, true, true) - price(S0, 100, T, false, true);
            check(b.lo <= d && d <= b.hi, "American parity bounds");
        }
    check(near(boundViolation(3.5, {4.0, 100.0}), 0.5) && near(boundViolation(101.0, {4.0, 100.0}), 1.0), "violation");

    for (double S0 : {80.0, 100.0, 120.0}) {
        check(near(price(S0, 100, 1, true, true), price(S0, 100, 1, true)), "C_A = C_E");
        check(price(S0, 100, 1, false, true) >= price(S0, 100, 1, false), "P_A >= P_E");
        check(price(S0, 100, 1, false, true) >= putPayoff(S0, 100), "P_A >= payoff");
    }
    double prev = 0;
    for (double T : {0.25, 0.5, 1.0, 2.0}) {
        const double p = price(100, 100, T, false, true);
        check(p >= prev, "P_A increasing in T");
        prev = p;
    }

    const double C1 = price(100, 90, 1, true), C2 = price(100, 100, 1, true), C3 = price(100, 110, 1, true);
    check(C1 > C2 && C2 > C3, "C decreasing in X");
    check(butterflyGap(C1, C2, C3, 90, 100, 110) <= 0, "convex in X");
    check(C1 - C2 <= 10 * std::exp(-0.05), "slope in X");
    check(near(butterflyGap(15.0, 10.0, 2.0, 90, 100, 110), 1.5), "butterfly arbitrage");

    if (failures == 0) std::printf("All ch07 tests passed\n");
    return failures == 0 ? 0 : 1;
}
