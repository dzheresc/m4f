#include "m4f/one_step.hpp"
#include "m4f/forward.hpp"
#include "m4f/fx_arbitrage.hpp"

#include <cmath>
#include <cstdio>

using namespace m4f::ch01;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

int main() {
    const OneStepMarket m(100, 110, 50, 60, 40, 0.8);
    check(near(m.r(), 0.1) && near(m.u(), 0.2) && near(m.d(), -0.2), "returns");
    check(m.isArbitrageFree(), "no arbitrage");
    check(near(m.riskNeutralProb(), 0.75), "p*");
    check(near(m.forwardPrice(), 55), "forward");

    const auto c = m.replicate(10, 0);
    check(near(c.x, 0.5) && near(c.y, -2.0 / 11), "call replication");
    check(near(m.price(10, 0), 75.0 / 11), "call price 6.8182");
    check(near(m.price(10, 0), m.riskNeutralPrice(10, 0)), "replication == risk-neutral");
    check(near(m.price(10, 0) - m.price(0, 10), 50 - 50 / 1.1), "put-call parity");

    for (double S0 : {55.0, 36.0}) {
        const OneStepMarket bad(100, 110, S0, 60, 40, 0.8);
        const auto P = bad.arbitrage();
        check(P && near(bad.value0(*P), 0) && bad.value1(*P, State::Up) >= 0 &&
                  bad.value1(*P, State::Down) >= 0 &&
                  bad.value1(*P, State::Up) + bad.value1(*P, State::Down) > 0,
              "arbitrage portfolio");
    }
    check(!m.arbitrage(), "no arbitrage portfolio in fair market");

    // Book: Examples 1.1-1.2, Exercises 1.1-1.2
    {
        const OneStepMarket ex(100, 110, 50, 52, 48, 0.5);
        const Portfolio P{20, 10};
        check(near(ex.u(), 0.04) && near(ex.d(), -0.04) && !ex.isArbitrageFree(), "Example 1.1");
        check(near(ex.value0(P), 2000) && near(ex.value1(P, State::Up), 2140) &&
                  near(ex.value1(P, State::Down), 2060) &&
                  near(ex.portfolioReturn(P, State::Up), 0.07) &&
                  near(ex.portfolioReturn(P, State::Down), 0.03),
              "Example 1.2");
        const OneStepMarket er(90, 100, 25, 30, 20, 0.5);
        const Portfolio Q{10, 15};
        check(near(er.value0(Q), 1600) && near(er.value1(Q, State::Up), 1800) &&
                  near(er.value1(Q, State::Down), 1700),
              "Exercise 1.1");
        const auto R = er.replicate(1160, 1040);
        check(near(R.x, 12) && near(R.y, 8) && near(er.value0(R), 1020), "Exercise 1.2");
    }

    // Exercise 1.3: USD -> GBP (A) -> EUR (B) -> USD (A) gains ~0.594%
    {
        const FxMarket fx({{"A", "EUR", "USD", 1.0202, 1.0284},
                           {"A", "GBP", "USD", 1.5718, 1.5844},
                           {"B", "EUR", "GBP", 0.6324, 0.6401},
                           {"B", "USD", "GBP", 0.6299, 0.6375}});
        const auto c = fx.bestCycle("USD");
        check(c.legs.size() == 3 && c.legs[0].to == "GBP" && c.legs[1].to == "EUR" &&
                  near(c.growth, 1.0202 / (1.5844 * 0.6401)),
              "Exercise 1.3");
    }

    // Example 1.4 / Proposition 1.1
    {
        const OneStepMarket m4(100, 110, 100, 125, 105, 0.5);
        check(near(m4.u(), 0.25) && near(m4.d(), 0.05) && m4.isArbitrageFree() &&
                  near(m4.riskNeutralProb(), 0.25) && near(m4.forwardPrice(), 110),
              "Example 1.4");
    }

    // Section 1.4 and Exercise 1.4
    {
        const OneStepMarket rr(100, 110, 80, 100, 60, 0.8);
        auto ev = [&](Portfolio P, double E, double sd) {
            const double ku = rr.portfolioReturn(P, State::Up), kd = rr.portfolioReturn(P, State::Down);
            return near(rr.expect(ku, kd), E) && near(rr.stddev(ku, kd), sd);
        };
        check(ev({50, 60}, 0.12, 0.08) && ev({0, 100}, 0.10, 0.0) && ev({125, 0}, 0.15, 0.20),
              "Section 1.4");
        const auto H = rr.split(10000, 0.5);
        check(near(H.x, 62.5) && near(H.y, 50) && ev(H, 0.125, 0.10), "Exercise 1.4");
    }

    // Section 1.5: Exercises 1.5 and 1.6
    {
        const auto P = forwardArbitrage(34, 100, 112, 38.60);
        check(P && near(value0(*P, 34, 100), 0) && near(value1(*P, 30, 112, 38.60), 0.52) &&
                  near(value1(*P, 50, 112, 38.60), 0.52),
              "Exercise 1.5");
        check(!forwardArbitrage(34, 100, 112, 38.08), "fair forward has no arbitrage");
        const double rf = impliedForeignRate(1.6, 1.5, 0.05);
        check(near(rf, 0.12) && near(100 / (1 + rf), 100 / 1.12), "Exercise 1.6");
    }

    // Section 1.6: Exercises 1.7-1.9
    {
        const OneStepMarket o(100, 110, 100, 120, 80, 0.5);
        check(near(o.price(20, 0), 150.0 / 11) && near(o.price(30, 0), 225.0 / 11) &&
                  near(o.price(10, 0), 75.0 / 11),
              "call X=100, Exercise 1.7");
        check(near(OneStepMarket(100, 105, 100, 120, 80, 0.5).price(20, 0), 50 - 4000.0 / 105) &&
                  near(OneStepMarket(100, 115, 100, 120, 80, 0.5).price(20, 0), 50 - 4000.0 / 115),
              "Exercise 1.8");
        check(near(o.price(0, 20), 50.0 / 11), "Exercise 1.9");
    }

    // Section 1.7: hedging a future purchase, Exercises 1.11-1.12
    {
        const OneStepMarket h(100, 110, 100, 160, 40, 0.5);
        check(near(h.price(60, 0), 350.0 / 11), "C(0)=31.8182");
        check(near(hedgedPurchaseCost(h, 100, 1, State::Up), 135) &&
                  near(hedgedPurchaseCost(h, 100, 1, State::Down), 75) &&
                  near(hedgedPurchaseCost(h, 100, 2, State::Up), 110) &&
                  near(hedgedPurchaseCost(h, 100, 2, State::Down), 110),
              "hedged cost");
        for (double p : {0.1, 0.25, 0.5, 0.75, 0.9}) {
            const OneStepMarket hp(100, 110, 100, 160, 40, p);
            const double s0 = hp.stddev(160, 40);
            const double s1 = hp.stddev(hedgedPurchaseCost(hp, 100, 1, State::Up),
                                        hedgedPurchaseCost(hp, 100, 1, State::Down));
            check(near(s1, s0 / 2), "Exercise 1.12");
        }
    }

    std::printf(failures ? "%d failure(s)\n" : "all tests passed\n", failures);
    return failures ? 1 : 0;
}
