#include "m4f/report.hpp"
#include "m4f/binomial.hpp"
#include "m4f/options.hpp"

#include <cmath>
#include <format>
#include <ostream>
#include <vector>

namespace m4f::ch07 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

std::string fl(const std::vector<double>& vs) {
    std::string s = "[";
    for (std::size_t i = 0; i < vs.size(); ++i) s += (i ? ", " : "") + f(vs[i]);
    return s + "]";
}

double price(double S0, double X, double T, bool isCall, bool american = false) {
    return crrPrice(S0, X, 0.05, 0.2, T, 200, isCall, american);
}

}  // namespace

void printReport(std::ostream& os) {
    os << "Chapter 7: Options: General Properties (illustrative numbers, book formulas)\n";

    os << "\n== 7.1 Definitions: X=100, premiums C=10 P=5, r=0.05 T=1 ==\n";
    for (double ST : {80.0, 100.0, 120.0}) {
        const double c = callPayoff(ST, 100.0), p = putPayoff(ST, 100.0);
        os << std::format("S(T)={}: call payoff={} profit long={} short={}; put payoff={} profit long={} short={}\n",
                          f(ST), f(c), f(profit(c, 10.0, 0.05, 1.0)), f(-profit(c, 10.0, 0.05, 1.0)), f(p),
                          f(profit(p, 5.0, 0.05, 1.0)), f(-profit(p, 5.0, 0.05, 1.0)));
    }

    os << "\n== 7.2 Put-call parity: C - P = S(0) - X e^(-rT), S(0)=100 X=100 r=0.05 T=1 ==\n";
    os << std::format("S(0) - X e^(-rT) = {}\n", f(100.0 - 100.0 * std::exp(-0.05)));
    for (const auto& [C, P] : std::vector<std::pair<double, double>>{{10.45, 5.57}, {12.0, 5.0}, {9.0, 6.0}}) {
        const double gap = parityGap(C, P, 100.0, 100.0, 0.05, 1.0);
        os << std::format("C={} P={}: gap={} arbitrage profit at T={}\n", f(C), f(P), f(gap),
                          f(std::abs(gap) * std::exp(0.05)));
    }
    os << std::format("Dividend 2 at t=0.5 (PV {}): C - P should be {}\n", f(2 * std::exp(-0.025)),
                      f(-parityGap(0.0, 0.0, 100.0, 100.0, 0.05, 1.0, 2 * std::exp(-0.025))));

    os << "\n== 7.3.1 Bounds on European options: S(0)=100 X=100 r=0.05 T=1 ==\n";
    const Bounds cb = europeanCallBounds(100.0, 100.0, 0.05, 1.0), pb = europeanPutBounds(100.0, 100.0, 0.05, 1.0);
    os << std::format("call in {}, put in {}\n", fl({cb.lo, cb.hi}), fl({pb.lo, pb.hi}));
    os << std::format("Quoted call 3.50: violation {}; quoted put 96.00: violation {}; model call {}: violation {}\n",
                      f(boundViolation(3.5, cb)), f(boundViolation(96.0, pb)), f(price(100.0, 100.0, 1.0, true)),
                      f(boundViolation(price(100.0, 100.0, 1.0, true), cb)));

    os << "\n== 7.3.2 American calls on a non-dividend paying stock (CRR, sigma=0.2, N=200) ==\n";
    for (double S0 : {90.0, 100.0, 120.0})
        os << std::format("S(0)={}: C_E={} C_A={} exercise now={}\n", f(S0), f(price(S0, 100.0, 1.0, true)),
                          f(price(S0, 100.0, 1.0, true, true)), f(callPayoff(S0, 100.0)));

    os << "\n== 7.3.3 American options: S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) ==\n";
    for (double S0 : {90.0, 100.0, 120.0}) {
        const double PE = price(S0, 100.0, 1.0, false), PA = price(S0, 100.0, 1.0, false, true);
        const double CA = price(S0, 100.0, 1.0, true, true);
        const Bounds b = americanParityBounds(S0, 100.0, 0.05, 1.0);
        os << std::format("S(0)={}: P_E={} P_A={} C_A-P_A={} in {}\n", f(S0), f(PE), f(PA), f(CA - PA),
                          fl({b.lo, b.hi}));
    }

    os << "\n== 7.4.1 European options: dependence on X and S(0) ==\n";
    const std::vector<double> Xs{90.0, 100.0, 110.0};
    std::vector<double> Cs, Ps;
    for (double X : Xs) {
        Cs.push_back(price(100.0, X, 1.0, true));
        Ps.push_back(price(100.0, X, 1.0, false));
    }
    os << std::format("X={}: C={} P={}\n", fl(Xs), fl(Cs), fl(Ps));
    os << std::format("C(90)-C(100)={} <= e^(-rT) x 10 = {}; butterfly gap={} (<= 0)\n", f(Cs[0] - Cs[1]),
                      f(10 * std::exp(-0.05)), f(butterflyGap(Cs[0], Cs[1], Cs[2], 90.0, 100.0, 110.0)));
    os << std::format("Quotes C=15, 10, 2: butterfly gap={} -> write 2 x C(100), buy C(90) and C(110)\n",
                      f(butterflyGap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0)));
    std::vector<double> CS;
    for (double S : {90.0, 100.0, 110.0}) CS.push_back(price(S, 100.0, 1.0, true));
    os << std::format("S(0)={}: C={}\n", fl({90.0, 100.0, 110.0}), fl(CS));

    os << "\n== 7.4.2 American options: dependence on T ==\n";
    const std::vector<double> Ts{0.25, 0.5, 1.0, 2.0};
    std::vector<double> PAs;
    for (double T : Ts) PAs.push_back(price(100.0, 100.0, T, false, true));
    os << std::format("T={}: P_A={}\n", fl(Ts), fl(PAs));

    os << "\n== 7.5 Time value of options: price - intrinsic value, X=100 T=1 ==\n";
    for (double S0 : {80.0, 100.0, 120.0}) {
        const double C = price(S0, 100.0, 1.0, true), P = price(S0, 100.0, 1.0, false);
        os << std::format("S(0)={}: call {} time value {}; put {} time value {}\n", f(S0), f(C),
                          f(C - callPayoff(S0, 100.0)), f(P), f(P - putPayoff(S0, 100.0)));
    }
}

}  // namespace m4f::ch07
