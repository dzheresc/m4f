#include "m4f/report.hpp"
#include "m4f/forward.hpp"
#include "m4f/fx_arbitrage.hpp"
#include "m4f/one_step.hpp"

#include <format>
#include <ostream>
#include <tuple>

namespace m4f::ch01 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

void portfolioLine(std::ostream& os, const OneStepMarket& m, const char* name, Portfolio P) {
    const double Ku = m.portfolioReturn(P, State::Up), Kd = m.portfolioReturn(P, State::Down);
    os << std::format("{}: x={} y={} V(0)={} V(1)={}|{} K_V={}|{}\n", name, f(P.x), f(P.y),
                      f(m.value0(P)), f(m.value1(P, State::Up)), f(m.value1(P, State::Down)),
                      f(Ku), f(Kd));
}


void riskLine(std::ostream& os, const OneStepMarket& m, const char* name, Portfolio P) {
    const double Ku = m.portfolioReturn(P, State::Up), Kd = m.portfolioReturn(P, State::Down);
    os << std::format("{}: x={} y={} V(1)={}|{} K_V={}|{} E(K_V)={} sigma_V={}\n", name, f(P.x),
                      f(P.y), f(m.value1(P, State::Up)), f(m.value1(P, State::Down)), f(Ku),
                      f(Kd), f(m.expect(Ku, Kd)), f(m.stddev(Ku, Kd)));
}

void optionLine(std::ostream& os, const OneStepMarket& m, const char* name, double Du,
                double Dd) {
    const auto R = m.replicate(Du, Dd);
    os << std::format("{}: payoff {}|{} replicate x={} y={} price={}\n", name, f(Du), f(Dd),
                      f(R.x), f(R.y), f(m.price(Du, Dd)));
}

}  // namespace

static void arbitrageLines(std::ostream& os, const OneStepMarket& m) {
    os << std::format("d < r < u: {} < {} < {} -> arbitrage-free: {}\n", f(m.d()), f(m.r()),
                      f(m.u()), m.isArbitrageFree() ? "yes" : "no");
    if (const auto P = m.arbitrage())
        os << std::format("Arbitrage: x={} y={} V(0)={} V(1)={}|{}\n", f(P->x), f(P->y),
                          f(m.value0(*P)), f(m.value1(*P, State::Up)),
                          f(m.value1(*P, State::Down)));
}

void printReport(std::ostream& os) {
    // Examples 1.1-1.2. The book leaves p unspecified (0 < p < 1); it does not
    // affect anything printed here.
    const OneStepMarket ex(100.0, 110.0, 50.0, 52.0, 48.0, 0.5);

    os << "== Example 1.1: returns ==\n";
    os << std::format("A(0)={} A(1)={} S(0)={} S(1)={}|{}\n", f(ex.A0()), f(ex.A1()),
                      f(ex.S0()), f(ex.S1(State::Up)), f(ex.S1(State::Down)));
    os << std::format("K_A={} K_S={}|{}\n", f(ex.r()), f(ex.u()), f(ex.d()));

    os << "\n== Example 1.2: portfolio value and return ==\n";
    portfolioLine(os, ex, "Portfolio", {20.0, 10.0});

    // Exercises 1.1-1.2 (again p is left unspecified).
    const OneStepMarket exr(90.0, 100.0, 25.0, 30.0, 20.0, 0.5);
    os << "\n== Exercise 1.1: A(0)=90 A(1)=100 S(0)=25 S(1)=30|20 ==\n";
    portfolioLine(os, exr, "Portfolio", {10.0, 15.0});

    os << "\n== Exercise 1.2: portfolio with V(1)=1160|1040 ==\n";
    portfolioLine(os, exr, "Solution", exr.replicate(1160.0, 1040.0));

    os << "\n== No-arbitrage check on the Example 1.1 market ==\n";
    arbitrageLines(os, ex);

    os << "\n== Exercise 1.3: currency dealers A (New York) and B (London) ==\n";
    const FxMarket fx({{"A", "EUR", "USD", 1.0202, 1.0284},
                       {"A", "GBP", "USD", 1.5718, 1.5844},
                       {"B", "EUR", "GBP", 0.6324, 0.6401},
                       {"B", "USD", "GBP", 0.6299, 0.6375}});
    const auto cyc = fx.bestCycle("USD");
    std::string route = "USD";
    for (const auto& l : cyc.legs) route += " -> " + l.to;
    os << "Best round trip: " << route << "\n";
    for (const auto& l : cyc.legs)
        os << std::format("  {} -> {} with dealer {}: rate {:.6f}\n", l.from, l.to, l.dealer, l.rate);
    os << std::format("1 USD -> {:.6f} USD: {}\n", cyc.growth,
                      cyc.growth > 1 ? "arbitrage (borrow USD, run the cycle, repay)" : "no arbitrage");

    // Example 1.4 / Figure 1.1
    // (p left unspecified by the book).
    const OneStepMarket m(100.0, 110.0, 100.0, 125.0, 105.0, 0.5);
    const auto Su = m.S1(State::Up), Sd = m.S1(State::Down);
    os << "\n== Example 1.4: one-step binomial tree (Figure 1.1) ==\n";
    os << std::format("A(0)={} A(1)={} S(0)={} S(1)={}|{}\n", f(m.A0()), f(m.A1()), f(m.S0()),
                      f(Su), f(Sd));
    os << std::format("K_A={} K_S={}|{}\n", f(m.r()), f(m.u()), f(m.d()));
    os << std::format("Proposition 1.1 (S(0)=A(0)): S^d < A(1) < S^u: {} < {} < {} -> {}\n",
                      f(Sd), f(m.A1()), f(Su), Sd < m.A1() && m.A1() < Su ? "holds" : "fails");
    // Proof of Proposition 1.1: the two violating cases (illustrative S(1) values).
    for (const auto& [up, down, label] : {std::tuple{130.0, 110.0, "A(1) <= S^d"},
                                         std::tuple{110.0, 90.0, "A(1) >= S^u"}}) {
        const OneStepMarket bad(100.0, 110.0, 100.0, up, down, 0.5);
        const auto P = *bad.arbitrage();
        os << std::format("  proof, case {} (S(1)={}|{}): x={} y={} V(0)={} V(1)={}|{}\n", label,
                          f(up), f(down), f(P.x), f(P.y), f(bad.value0(P)),
                          f(bad.value1(P, State::Up)), f(bad.value1(P, State::Down)));
    }
    arbitrageLines(os, m);


    // Section 1.4: risk and return, $10,000 to invest.
    const OneStepMarket rr(100.0, 110.0, 80.0, 100.0, 60.0, 0.8);
    os << "\n== 1.4 Risk and return: A=100->110, S(0)=80, S(1)=100|60, p=0.8, budget 10000 ==\n";
    riskLine(os, rr, "Mixed", {50.0, 60.0});
    riskLine(os, rr, "Bonds only", {0.0, 100.0});
    riskLine(os, rr, "Stock only", {125.0, 0.0});
    riskLine(os, rr, "Exercise 1.4 (50/50)", rr.split(10000.0, 0.5));

    os << "\n== 1.5 Forward contracts ==\n";
    os << std::format("Example 1.5: F={}\n", f(80.0));
    for (double S1 : {84.0, 75.0})
        os << std::format("  S(1)={}: long payoff {}, short payoff {}\n", f(S1),
                          f(longForwardPayoff(S1, 80.0)), f(shortForwardPayoff(S1, 80.0)));
    os << std::format("Proposition 1.2: S(0)=50 A(0)=100 A(1)=110 -> F = S(0)(1+r) = {}\n",
                      f(forwardPrice(50.0, 100.0, 110.0)));
    {
        const double S0 = 34.0, A0 = 100.0, A1 = 112.0, F = 38.60;
        os << std::format("Exercise 1.5: A=100->112, S(0)=34, quoted F={}, fair F={}\n", f(F),
                          f(forwardPrice(S0, A0, A1)));
        const auto P = *forwardArbitrage(S0, A0, A1, F);
        os << std::format("  arbitrage (x,y,z)=({},{},{}) V(0)={} V(1)={}|{}|{} for S(1)=30|40|50\n",
                          f(P.x), f(P.y), f(P.z), f(value0(P, S0, A0)), f(value1(P, 30.0, A1, F)),
                          f(value1(P, 40.0, A1, F)), f(value1(P, 50.0, A1, F)));
    }
    {
        const double rf = impliedForeignRate(1.6, 1.50, 0.05);
        os << std::format("Exercise 1.6: A=100->105 USD, S(0)=1.6 USD/GBP, F=1.50 -> sterling rate {}, "
                          "bond paying GBP 100 costs GBP {}\n", f(rf), f(100.0 / (1 + rf)));
    }

    // Section 1.6: call and put options (p left unspecified by the book).
    const OneStepMarket o(100.0, 110.0, 100.0, 120.0, 80.0, 0.5);
    const auto Ou = o.S1(State::Up), Od = o.S1(State::Down);
    os << "\n== 1.6 Call and put options: A=100->110, S(0)=100, S(1)=120|80 ==\n";
    optionLine(os, o, "Call X=100", callPayoff(Ou, 100.0), callPayoff(Od, 100.0));
    for (double X : {90.0, 110.0})
        optionLine(os, o, std::format("Exercise 1.7 call X={:.0f}", X).c_str(), callPayoff(Ou, X),
                   callPayoff(Od, X));
    for (double A1 : {105.0, 115.0}) {
        const OneStepMarket o8(100.0, A1, 100.0, 120.0, 80.0, 0.5);
        optionLine(os, o8, std::format("Exercise 1.8 call X=100, A(1)={:.0f}", A1).c_str(),
                   callPayoff(Ou, 100.0), callPayoff(Od, 100.0));
    }
    optionLine(os, o, "Exercise 1.9 put X=100", putPayoff(Ou, 100.0), putPayoff(Od, 100.0));
    const double X = 100.0;
    const double Cu = callPayoff(Ou, X), Cd = callPayoff(Od, X);
    const double Pu = putPayoff(Ou, X), Pd = putPayoff(Od, X);
    const double C0 = o.price(Cu, Cd), P0 = o.price(Pu, Pd);
    os << std::format("Risk-neutral p*={} -> C(0)={} P(0)={}\n", f(o.riskNeutralProb()),
                      f(o.riskNeutralPrice(Cu, Cd)), f(o.riskNeutralPrice(Pu, Pd)));
    os << std::format("Put-call parity: C(0)-P(0)={} S(0)-X/(1+r)={}\n", f(C0 - P0),
                      f(o.S0() - X / (1 + o.r())));

    os << "\n== 1.7 Managing risk with options (section 1.6 market, budget 1000) ==\n";
    const double budget = 1000.0;
    const double nS = budget / o.S0(), nC = budget / C0;
    os << std::format("All in stock: {} shares -> V(1)={}|{} return {}|{}\n", f(nS),
                      f(nS * Ou), f(nS * Od), f(nS * Ou / budget - 1), f(nS * Od / budget - 1));
    os << std::format("All in calls: {} calls  -> V(1)={}|{} return {}|{}\n", f(nC),
                      f(nC * Cu), f(nC * Cd), f(nC * Cu / budget - 1), f(nC * Cd / budget - 1));
    {
        const double hS = budget / 2 / o.S0(), hC = budget / 2 / C0;
        const double Vu = hS * Ou + hC * Cu, Vd = hS * Od + hC * Cd;
        os << std::format("Exercise 1.10 (50/50): {} shares + {} calls -> V(1)={}|{} return {}|{}\n",
                          f(hS), f(hC), f(Vu), f(Vd), f(Vu / budget - 1), f(Vd / budget - 1));
    }

    // Hedging a future purchase with calls (strike 100).
    const OneStepMarket h(100.0, 110.0, 100.0, 160.0, 40.0, 0.5);
    os << "\nHedging a future purchase: A=100->110, S(0)=100, S(1)=160|40, call X=100\n";
    os << std::format("C(0)={} -> loan repaid at t=1: {}\n", f(h.price(60.0, 0.0)),
                      f(h.price(60.0, 0.0) * (1 + h.r())));
    for (double n : {0.0, 1.0, 2.0})
        os << std::format("Cost of one share with {:.0f} calls: {}|{}\n", n,
                          f(hedgedPurchaseCost(h, 100.0, n, State::Up)),
                          f(hedgedPurchaseCost(h, 100.0, n, State::Down)));
    for (double p : {0.25, 0.5, 0.75}) {
        const OneStepMarket hp(100.0, 110.0, 100.0, 160.0, 40.0, p);
        const double s0 = hp.stddev(hedgedPurchaseCost(hp, 100.0, 0.0, State::Up),
                                    hedgedPurchaseCost(hp, 100.0, 0.0, State::Down));
        const double s1 = hp.stddev(hedgedPurchaseCost(hp, 100.0, 1.0, State::Up),
                                    hedgedPurchaseCost(hp, 100.0, 1.0, State::Down));
        os << std::format("Exercise 1.11/1.12 p={}: sigma without option={} with option={} ratio={}\n",
                          f(p), f(s0), f(s1), f(s1 / s0));
    }
}

}  // namespace m4f::ch01
