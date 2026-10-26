#include "m4f/black_scholes.hpp"
#include "m4f/hedging.hpp"
#include "m4f/risk.hpp"

#include <cmath>
#include <cstdio>

using namespace m4f::ch09;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b, double eps = 1e-7) { return std::abs(a - b) < eps; }

int main() {
    const double S = 100.0, X = 100.0, r = 0.05, sigma = 0.2, T = 0.5;

    check(near(normCdf(0.0), 0.5), "N(0)");
    check(near(normCdf(1.959963984540054), 0.975, 1e-9), "N(1.96)");
    check(near(normInv(0.975), 1.959963984540054, 1e-9), "N^-1(0.975)");
    check(near(normPdf(0.0), 1 / std::sqrt(2 * M_PI), 1e-12), "pdf(0)");

    // Greeks against finite differences
    const double h = 1e-4;
    for (bool isCall : {true, false}) {
        const Greeks g = bsGreeks(S, X, r, sigma, T, isCall);
        auto p = [&](double s, double x, double rr, double sg, double t) { return bsPrice(s, x, rr, sg, t, isCall); };
        check(near(g.price, p(S, X, r, sigma, T)), "price");
        check(near(g.delta, (p(S + h, X, r, sigma, T) - p(S - h, X, r, sigma, T)) / (2 * h), 1e-6), "delta");
        check(near(g.gamma, (p(S + h, X, r, sigma, T) - 2 * p(S, X, r, sigma, T) + p(S - h, X, r, sigma, T)) / (h * h), 1e-4),
              "gamma");
        check(near(g.theta, -(p(S, X, r, sigma, T + h) - p(S, X, r, sigma, T - h)) / (2 * h), 1e-5), "theta");
        check(near(g.vega, (p(S, X, r, sigma + h, T) - p(S, X, r, sigma - h, T)) / (2 * h), 1e-5), "vega");
        check(near(g.rho, (p(S, X, r + h, sigma, T) - p(S, X, r - h, sigma, T)) / (2 * h), 1e-5), "rho");
    }

    // hedges are neutral and self-financing
    const std::vector<Position> written = {{-1000.0, X, T, true}};
    const Position extra = {0.0, 110.0, T, true};
    const Hedge hd = deltaHedge(written, S, r, sigma);
    const Hedge hg = deltaGreekHedge(written, extra, S, r, sigma, Greek::Gamma);
    const Hedge hv = deltaGreekHedge(written, extra, S, r, sigma, Greek::Vega);
    for (const Hedge* hh : {&hd, &hg, &hv}) {
        check(near(portfolioValue(*hh, S, 0.0, r, sigma), 0.0, 1e-8), "hedge value 0 at t=0");
        check(near(totalGreek(hh->options, S, r, sigma, Greek::Delta) + hh->shares, 0.0, 1e-9), "delta neutral");
    }
    check(near(totalGreek(hg.options, S, r, sigma, Greek::Gamma), 0.0, 1e-9), "gamma neutral");
    check(near(totalGreek(hv.options, S, r, sigma, Greek::Vega), 0.0, 1e-6), "vega neutral");
    check(std::abs(portfolioValue(hd, S + 1, 0.0, r, sigma) - totalGreek(written, S, r, sigma, Greek::Gamma) / 2) < 0.5,
          "delta hedge error ~ gamma/2");

    // rebalancing is self-financing
    const auto steps = rebalancePath(written, {100.0, 103.0, 101.0}, 1.0 / 52, r, sigma);
    check(steps.size() == 3 && near(steps[0].value, 0.0, 1e-8), "path start");
    check(near(steps[1].money, steps[0].money * std::exp(r / 52) - (steps[1].shares - steps[0].shares) * steps[1].S, 1e-6),
          "path self-financing");

    // lognormal
    const Lognormal m = {100.0, 0.08, 0.3, 0.5};
    check(near(m.cdf(m.quantile(0.3)), 0.3, 1e-9), "quantile/cdf");
    check(near(m.expectedCallPayoff(90.0) - m.expectedPutPayoff(90.0), m.expectedStock() - 90.0, 1e-9), "parity");
    {
        const int n = 20000;
        const double lo = -8.0, hi = 8.0;
        double tot = 0.0;
        for (int i = 0; i < n; ++i) {
            const double z = lo + (i + 0.5) * (hi - lo) / n;
            const double s = 100.0 * std::exp(0.08 * 0.5 + 0.3 * std::sqrt(0.5) * z);
            tot += std::max(s - 90.0, 0.0) * normPdf(z) * (hi - lo) / n;
        }
        check(near(tot, m.expectedCallPayoff(90.0), 1e-6), "expected call payoff by quadrature");
    }

    // VaR and strategies
    const Strategy stock = unhedgedHolding(1.0);
    check(near(valueAtRisk(stock, m, r, 0.3, 0.95), 100.0 - m.quantile(0.05)), "VaR stock");
    const Strategy hedged = putHedgedHolding("p", 1.0, 95.0);
    check(near(valueAtRisk(hedged, m, r, 0.3, 0.95), bsPrice(100.0, 95.0, r, 0.3, 0.5, false) + 5.0), "VaR hedged");
    check(expectedReturn(hedged, m, r, 0.3) < expectedReturn(stock, m, r, 0.3), "hedging costs return");
    const double V0 = 10000.0;
    for (const Strategy& st : {allInStock(V0, 100.0), allInCalls("c", V0, 100.0, 100.0, r, 0.25, 0.5),
                               bullSpread("b", V0, 100.0, 100.0, 115.0, r, 0.25, 0.5)}) {
        check(near(initialCost(st, 100.0, r, 0.25, 0.5), V0), "initial cost = V0");
        const double be = breakeven(st, m, r, 0.25);
        check(near(st.terminalValue(be, r, 0.5), V0, 1e-6), "breakeven");
        check(near(probLoss(st, m, r, 0.25), m.cdf(be)), "P(loss)");
    }
    check(near(breakeven(allInCalls("c", V0, 100.0, 100.0, r, 0.25, 0.5), m, r, 0.25),
               100.0 + bsPrice(100.0, 100.0, r, 0.25, 0.5, true), 1e-6),
          "call breakeven = X + C");

    if (failures == 0) std::printf("All C++ tests passed\n");
    return failures == 0 ? 0 : 1;
}
