#include "m4f/report.hpp"
#include "m4f/black_scholes.hpp"
#include "m4f/hedging.hpp"
#include "m4f/risk.hpp"

#include <format>
#include <ostream>
#include <string>
#include <vector>

namespace m4f::ch09 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

std::string fl(const std::vector<double>& vs) {
    std::string s = "[";
    for (std::size_t i = 0; i < vs.size(); ++i) s += (i ? ", " : "") + f(vs[i]);
    return s + "]";
}

std::vector<double> legQtys(const Strategy& st) {
    std::vector<double> q;
    for (const OptionLeg& l : st.legs) q.push_back(l.qty);
    return q;
}

const char* greekName(Greek g) {
    switch (g) {
        case Greek::Delta: return "delta";
        case Greek::Gamma: return "gamma";
        case Greek::Theta: return "theta";
        case Greek::Vega: return "vega";
        case Greek::Rho: return "rho";
    }
    return "";
}

}  // namespace

void printReport(std::ostream& os) {
    os << "Chapter 9: Financial Engineering (illustrative numbers, book formulas)\n";

    const double S0 = 100.0, X = 100.0, r = 0.05, sigma = 0.2, T = 0.5;
    const std::vector<double> moves = {90.0, 95.0, 99.0, 100.0, 101.0, 105.0, 110.0};

    os << "\n== 9.1.1 Delta hedging: written 1000 calls X=100 T=0.5, S=100 r=0.05 sigma=0.2 ==\n";
    const std::vector<Position> written = {{-1000.0, X, T, true}};
    const double C = bsPrice(S0, X, r, sigma, T, true);
    const Hedge h = deltaHedge(written, S0, r, sigma);
    os << std::format("Call price C={}, delta={}\n", f(C), f(bsGreeks(S0, X, r, sigma, T, true).delta));
    os << std::format("Premium received {}; buy {} shares, money market {} (negative = borrow)\n", f(1000.0 * C),
                      f(h.shares), f(h.money));
    os << "Value of the position right after a jump in S (t=0): naked writer vs delta hedged\n";
    for (double S : moves) {
        const double nakedV = 1000.0 * C - 1000.0 * bsPrice(S, X, r, sigma, T, true);
        os << std::format("S={}: naked {} hedged {}\n", f(S), f(nakedV), f(portfolioValue(h, S, 0.0, r, sigma)));
    }
    os << "Same jumps one week later (t=1/52), no rebalancing:\n";
    std::string row = " ";
    for (double S : moves) row += std::format(" S={}:{}", f(S), f(portfolioValue(h, S, 1.0 / 52, r, sigma)));
    os << row << "\n";

    os << "\n== 9.1.1 Dynamic delta hedging along a weekly path, rebalanced each week ==\n";
    const std::vector<double> path = {100.0, 103.0, 101.0, 98.0, 102.0, 104.0};
    for (const RebalanceStep& st : rebalancePath(written, path, 1.0 / 52, r, sigma))
        os << std::format("t={} S={}: shares={} money={} value={}\n", f(st.t), f(st.S), f(st.shares), f(st.money),
                          f(st.value));

    os << "\n== 9.1.2 Greek parameters: S=100 X=100 r=0.05 sigma=0.2 T=0.5 ==\n";
    for (bool isCall : {true, false}) {
        const Greeks g = bsGreeks(S0, X, r, sigma, T, isCall);
        os << std::format("{}: price={} delta={} gamma={} theta={} vega={} rho={}\n", isCall ? "call" : "put",
                          f(g.price), f(g.delta), f(g.gamma), f(g.theta), f(g.vega), f(g.rho));
    }
    const std::vector<Greek> all = {Greek::Delta, Greek::Gamma, Greek::Theta, Greek::Vega, Greek::Rho};
    row = "Position Greeks of the written calls:";
    for (Greek g : all) row += std::format(" {}={}", greekName(g), f(totalGreek(written, S0, r, sigma, g)));
    os << row << "\n";

    os << "\n== 9.1.2 Delta-gamma and delta-vega hedging with calls X=110 T=0.5 ==\n";
    const Position extra = {0.0, 110.0, T, true};
    const Hedge hg = deltaGreekHedge(written, extra, S0, r, sigma, Greek::Gamma);
    const Hedge hv = deltaGreekHedge(written, extra, S0, r, sigma, Greek::Vega);
    const std::vector<std::pair<const char*, const Hedge*>> hedges = {{"delta", &h}, {"delta-gamma", &hg},
                                                                     {"delta-vega", &hv}};
    for (const auto& [label, hh] : hedges) {
        const double opt2 = hh->options.size() > 1 ? hh->options[1].qty : 0.0;
        os << std::format("{}: shares={} calls X=110: {} money={} gamma={} vega={}\n", label, f(hh->shares), f(opt2),
                          f(hh->money), f(totalGreek(hh->options, S0, r, sigma, Greek::Gamma)),
                          f(totalGreek(hh->options, S0, r, sigma, Greek::Vega)));
    }
    os << "Value after an immediate jump in S:\n";
    for (double S : moves)
        os << std::format("S={}: delta {} delta-gamma {} delta-vega {}\n", f(S), f(portfolioValue(h, S, 0.0, r, sigma)),
                          f(portfolioValue(hg, S, 0.0, r, sigma)), f(portfolioValue(hv, S, 0.0, r, sigma)));
    os << "Value after an immediate change in sigma (S unchanged):\n";
    for (double sg : {0.15, 0.2, 0.25, 0.3})
        os << std::format("sigma={}: delta {} delta-gamma {} delta-vega {}\n", f(sg),
                          f(portfolioValue(h, S0, 0.0, r, sg)), f(portfolioValue(hg, S0, 0.0, r, sg)),
                          f(portfolioValue(hv, S0, 0.0, r, sg)));

    os << "\n== 9.1.3 Applications: after one week S=97 and sigma=0.22 ==\n";
    for (const auto& [label, hh] : hedges)
        os << std::format("{}: {}\n", label, f(portfolioValue(*hh, 97.0, 1.0 / 52, r, 0.22)));
    os << "Theta: value change over one week with S and sigma unchanged\n";
    for (const auto& [label, hh] : hedges)
        os << std::format("{}: {}  (theta x dt = {})\n", label, f(portfolioValue(*hh, S0, 1.0 / 52, r, sigma)),
                          f(totalGreek(hh->options, S0, r, sigma, Greek::Theta) / 52));

    os << "\n== 9.2.1 Value at Risk: 1 share, S(0)=100, m=0.08 sigma=0.3 T=0.5 (lognormal) ==\n";
    const Lognormal model = {100.0, 0.08, 0.3, 0.5};
    const Strategy stock = unhedgedHolding(1.0);
    os << std::format("E S(T)={}\n", f(model.expectedStock()));
    for (double conf : {0.9, 0.95, 0.99})
        os << std::format("confidence {}: z={} S quantile={} VaR={}\n", f(conf), f(normInv(1 - conf)),
                          f(model.quantile(1 - conf)), f(valueAtRisk(stock, model, r, sigma, conf)));
    os << "Hedging with puts (priced with r=0.05 sigma=0.3), VaR at 95%:\n";
    for (double Xp : {85.0, 90.0, 95.0, 100.0}) {
        const Strategy st = putHedgedHolding(std::format("put X={}", static_cast<int>(Xp)), 1.0, Xp);
        os << std::format("X={}: put price={} initial value={} VaR={} E V(T)={} expected return={}\n", f(Xp),
                          f(bsPrice(100.0, Xp, r, 0.3, 0.5, false)), f(initialCost(st, 100.0, r, 0.3, 0.5)),
                          f(valueAtRisk(st, model, r, 0.3, 0.95)), f(st.expectedValue(model, r)),
                          f(expectedReturn(st, model, r, 0.3)));
    }
    const Strategy half = putHedgedHolding("half hedged X=95", 1.0, 95.0, 0.5);
    os << std::format("Half hedged X=95: VaR={} expected return={}\n", f(valueAtRisk(half, model, r, 0.3, 0.95)),
                      f(expectedReturn(half, model, r, 0.3)));

    os << "\n== 9.2.2 Case study: 10000 shares to be sold in 6 months, VaR(95%) target 100000 ==\n";
    const Strategy big = unhedgedHolding(10000.0);
    os << std::format("Unhedged: VaR={} expected return={}\n", f(valueAtRisk(big, model, r, 0.3, 0.95)),
                      f(expectedReturn(big, model, r, 0.3)));
    std::string chosen;
    for (double Xp : {80.0, 85.0, 90.0, 95.0, 100.0, 105.0}) {
        const Strategy st = putHedgedHolding(std::format("put X={}", static_cast<int>(Xp)), 10000.0, Xp);
        const double var = valueAtRisk(st, model, r, 0.3, 0.95);
        const double cost = 10000.0 * bsPrice(100.0, Xp, r, 0.3, 0.5, false);
        os << std::format("puts X={}: cost={} VaR={} expected return={}\n", f(Xp), f(cost), f(var),
                          f(expectedReturn(st, model, r, 0.3)));
        if (chosen.empty() && var <= 100000.0) chosen = st.name;
    }
    os << std::format("Cheapest hedge meeting the target: {}\n", chosen);

    os << "\n== 9.3.1 Speculating: capital 10000, S(0)=100 m=0.10 sigma=0.25 T=0.5, r=0.05 ==\n";
    const Lognormal spec = {100.0, 0.10, 0.25, 0.5};
    {
        const Strategy sS = allInStock(10000.0, 100.0);
        const Strategy c100 = allInCalls("calls X=100", 10000.0, 100.0, 100.0, r, 0.25, 0.5);
        const Strategy c110 = allInCalls("calls X=110", 10000.0, 100.0, 110.0, r, 0.25, 0.5);
        const Strategy halfMix = mix("half stock, half calls X=100", sS, 0.5, c100, 0.5);
        const std::vector<Strategy> strategies = {sS, c100, c110, halfMix};
        os << std::format("C(X=100)={} C(X=110)={}\n", f(bsPrice(100.0, 100.0, r, 0.25, 0.5, true)),
                          f(bsPrice(100.0, 110.0, r, 0.25, 0.5, true)));
        for (const Strategy& st : strategies)
            os << std::format("{}: shares={} options={}\n", st.name, f(st.shares), fl(legQtys(st)));
        const std::vector<double> scen = {80.0, 90.0, 100.0, 110.0, 120.0, 130.0};
        os << "Terminal value by scenario S(T)=" << fl(scen) << ":\n";
        for (const Strategy& st : strategies) {
            std::vector<double> vals;
            for (double S : scen) vals.push_back(st.terminalValue(S, r, 0.5));
            os << std::format("{}: {}\n", st.name, fl(vals));
        }
        os << "Expected return, probability of loss, breakeven S(T), VaR at 95%:\n";
        for (const Strategy& st : strategies)
            os << std::format("{}: E ret={} P(loss)={} breakeven={} VaR={}\n", st.name, f(expectedReturn(st, spec, r, 0.25)),
                              f(probLoss(st, spec, r, 0.25)), f(breakeven(st, spec, r, 0.25)),
                              f(valueAtRisk(st, spec, r, 0.25, 0.95)));
    }

    os << "\n== 9.3.2 Case study: expecting a rise to about 115 in 3 months (m=0.4 sigma=0.25 T=0.25) ==\n";
    const Lognormal bull = {100.0, 0.4, 0.25, 0.25};
    const Strategy sS = allInStock(10000.0, 100.0);
    const Strategy c100 = allInCalls("calls X=100", 10000.0, 100.0, 100.0, r, 0.25, 0.25);
    const Strategy bs = bullSpread("bull spread 100/115", 10000.0, 100.0, 100.0, 115.0, r, 0.25, 0.25);
    os << std::format("E S(T)={} P(S(T)>115)={}\n", f(bull.expectedStock()), f(1 - bull.cdf(115.0)));
    for (const Strategy& st : {sS, c100, bs})
        os << std::format("{}: options={} E ret={} P(loss)={} VaR={} value at S(T)=115: {}\n", st.name, fl(legQtys(st)),
                          f(expectedReturn(st, bull, r, 0.25)), f(probLoss(st, bull, r, 0.25)),
                          f(valueAtRisk(st, bull, r, 0.25, 0.95)), f(st.terminalValue(115.0, r, 0.25)));
    row = "If instead the stock stays flat (S(T)=100):";
    for (const Strategy& st : {sS, c100, bs}) row += std::format(" {}={}", st.name, f(st.terminalValue(100.0, r, 0.25)));
    os << row << "\n";
}

}  // namespace m4f::ch09
