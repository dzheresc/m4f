#include "m4f/report.hpp"
#include "m4f/forwards.hpp"
#include "m4f/futures.hpp"

#include <cmath>
#include <format>
#include <ostream>

namespace m4f::ch06 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

std::string fl(const Vec& vs) {
    std::string s = "[";
    for (std::size_t i = 0; i < vs.size(); ++i) s += (i ? ", " : "") + f(vs[i]);
    return s + "]";
}

}  // namespace

void printReport(std::ostream& os) {
    os << "Chapter 6: Forward and Futures Contracts (illustrative numbers, book formulas)\n";

    os << "\n== 6.1.1 Forward price: F(0,T) = S(0) e^(rT), S(0)=100 r=0.08 T=0.75 ==\n";
    const double F0 = forwardPrice(100.0, 0.08, 0.75);
    os << std::format("No dividends: F(0,T)={}\n", f(F0));
    os << std::format("Dividends 2 at t=0.25 and 2 at t=0.5: F(0,T)={}\n",
                      f(forwardPriceDividends(100.0, 0.08, 0.75, {{2.0, 0.25}, {2.0, 0.5}})));
    os << std::format("Dividend yield 3%: F(0,T)={}\n", f(forwardPriceYield(100.0, 0.08, 0.03, 0.75)));
    os << std::format("Currency 1.25 USD per EUR, r_USD=0.05 r_EUR=0.03, T=1: F={}\n",
                      f(forwardPriceYield(1.25, 0.05, 0.03, 1.0)));
    for (double Fq : {108.0, 104.0}) {
        const char* side = Fq > F0 ? "cash and carry: buy stock, sell forward" : "reverse: short stock, buy forward";
        os << std::format("Quoted F={}: {}, profit at T={}\n", f(Fq), side,
                          f(forwardArbitrageProfit(Fq, 100.0, 0.08, 0.75)));
    }

    os << "\n== 6.1.2 Value of a forward contract: V(t) = (F(t,T) - F(0,T)) e^(-r(T-t)) ==\n";
    for (double St : {104.0, 98.0}) {
        const double Ft = forwardPrice(St, 0.08, 0.25);
        os << std::format("t=0.5 S(t)={}: F(t,T)={} long V(t)={} short V(t)={}\n", f(St), f(Ft),
                          f(longForwardValue(F0, Ft, 0.08, 0.25)), f(-longForwardValue(F0, Ft, 0.08, 0.25)));
    }

    os << "\n== 6.2.1 Futures pricing: f(n) = E*(S(N) | S(n)), S(0)=100 u=0.10 d=-0.05 r=0.03 N=3 ==\n";
    const auto tree = futuresTree(100.0, 0.10, -0.05, 0.03, 3);
    for (int n = 0; n < 4; ++n) {
        Vec fut, fwd;
        for (int k = n; k >= 0; --k) {
            fut.push_back(tree[n][k]);
            fwd.push_back(100.0 * std::pow(1.1, k) * std::pow(0.95, n - k) * std::pow(1.03, 3 - n));
        }
        os << std::format("n={}: futures={} forward={}\n", n, fl(fut), fl(fwd));
    }
    os << "Marking to market: 100 units, initial margin 1000, maintenance 750\n";
    const Vec prices{106.0, 104.5, 101.0, 103.5, 99.0, 102.0};
    const auto days = marginAccount(prices, 100.0, 1000.0, 750.0);
    for (std::size_t n = 1; n <= days.size(); ++n)
        os << std::format("day {}: f={} balance={} margin call={}\n", n, f(prices[n]), f(days[n - 1].balance),
                          f(days[n - 1].call));

    os << "\n== 6.2.2 Hedging with futures: four equally likely scenarios ==\n";
    const Vec dS{-6.0, -1.0, 2.0, 5.0}, df{-5.0, -2.0, 3.0, 4.0}, p{0.25, 0.25, 0.25, 0.25};
    const double h = minVarianceHedgeRatio(dS, df, p);
    os << std::format("dS={} df={}\n", fl(dS), fl(df));
    os << std::format("h*={}; std of spot change: unhedged {}, h=1 {}, h* {}\n", f(h), f(hedgedStd(0.0, dS, df, p)),
                      f(hedgedStd(1.0, dS, df, p)), f(hedgedStd(h, dS, df, p)));
    os << std::format("Perfect hedge: buy at T with a long futures at f(0)=106; spot at T 112 or 95 -> cost {}\n",
                      fl({112.0 - (112.0 - 106.0), 95.0 - (95.0 - 106.0)}));
}

}  // namespace m4f::ch06
