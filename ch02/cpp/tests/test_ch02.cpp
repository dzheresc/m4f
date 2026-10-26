#include "m4f/interest.hpp"
#include "m4f/money_market.hpp"

#include <cmath>
#include <cstdio>

using namespace m4f::ch02;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b, double eps = 1e-9) { return std::abs(a - b) < eps; }

int main() {
    // Library identities
    check(near(simplePresentValue(simpleValue(100, 0.1, 0.5), 0.1, 0.5), 100), "simple PV");
    check(near(simpleRate(100, simpleValue(100, 0.08, 2), 2), 0.08), "simple rate");
    check(near(effectiveRate(0.1, 1), 0.1), "effective m=1");
    check(near(periodicFromContinuous(continuousFromPeriodic(0.1, 12), 12), 0.1), "rate roundtrip");
    check(near(periodicFromPeriodic(periodicFromPeriodic(0.1, 4, 12), 12, 4), 0.1), "m->k->m");
    check(near(continuousEffectiveRate(continuousFromPeriodic(0.1, 1)), 0.1), "continuous effective");
    check(std::abs(periodicValue(100, 0.1, 1000000, 1) - continuousValue(100, 0.1, 1)) < 1e-4,
          "continuous limit");
    check(near(annuityPresentValue(loanInstallment(1000, 0.1, 5), 0.1, 5), 1000), "loan");
    check(near(annuityFutureValue(1, 0.1, 5), annuityFactor(0.1, 5) * std::pow(1.1, 5)), "FV");
    for (auto m : {std::optional<int>{1}, std::optional<int>{2}, std::optional<int>{12},
                   std::optional<int>{}})
        check(near(impliedRate(zeroCouponPrice(0.08, 2.5, m), 2.5, m), 0.08), "implied rate");
    check(near(yieldToMaturity(couponBondPrice(100, 10, 3, 0.12), 100, 10, 3), 0.12, 1e-8), "YTM");
    check(near(couponBondPrice(100, 10, 3, 0.10), 100), "par bond");

    // Report values
    check(near(annuityPresentValue(1000, 0.10, 10), 6144.567105, 1e-6), "annuity PV");
    check(near(loanInstallment(100000, 0.005, 300), 644.301401, 1e-6), "mortgage installment");
    check(near(continuousFromPeriodic(0.12, 12), 0.119404, 1e-6), "12% monthly -> continuous");
    check(near(moneyMarketRollover(1, {0.5, 0.5}), 4), "rollover");

    std::printf(failures ? "%d failure(s)\n" : "all tests passed\n", failures);
    return failures ? 1 : 0;
}
