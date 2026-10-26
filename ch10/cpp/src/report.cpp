#include "m4f/report.hpp"
#include "m4f/bonds.hpp"
#include "m4f/term_structure.hpp"

#include <cmath>
#include <format>
#include <ostream>
#include <string>
#include <vector>

namespace m4f::ch10 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

std::string fl(const std::vector<double>& vs) {
    std::string s = "[";
    for (std::size_t i = 0; i < vs.size(); ++i) s += (i ? ", " : "") + f(vs[i]);
    return s + "]";
}

}  // namespace

void printReport(std::ostream& os) {
    os << "Chapter 10: Variable Interest Rates (illustrative numbers, book formulas)\n";

    os << "\n== 10.1.1 Investment in single bonds: F=100, 5% annual coupon, 4 years, y=0.06 continuous ==\n";
    const Bond bond = couponBond(100.0, 5.0, 4);
    const double y0 = 0.06;
    os << std::format("Price at y=0.06: {}\n", f(price(bond, y0)));
    os << "Yield jumps right after purchase; value of the investment at horizon t (coupons reinvested):\n";
    for (double y : {0.05, 0.06, 0.07}) {
        std::string row = std::format("y={}:", f(y));
        for (int t = 1; t <= 4; ++t) row += std::format(" t={}:{}", t, f(investmentValue(bond, y, t)));
        os << row << "\n";
    }
    os << "Annualised log return over [0,t]:\n";
    for (double y : {0.05, 0.06, 0.07}) {
        std::string row = std::format("y={}:", f(y));
        for (int t = 1; t <= 4; ++t) row += std::format(" t={}:{}", t, f(logReturn(bond, y0, y, t)));
        os << row << "\n";
    }

    os << "\n== 10.1.2 Duration: D = sum t_i C_i e^(-y t_i) / P, dP/dy = -D P ==\n";
    const double D = duration(bond, y0);
    os << std::format("Coupon bond: D={} years; zero-coupon 4y: D={}\n", f(D), f(duration(zeroCoupon(100.0, 4.0), y0)));
    const double h = 1e-4;
    os << std::format("dP/dy by finite difference: {}, -D P = {}\n", f((price(bond, y0 + h) - price(bond, y0 - h)) / (2 * h)),
                      f(-D * price(bond, y0)));
    os << "Value at the horizon t=D for y=0.05, 0.06, 0.07 (immunised to first order): "
       << fl({investmentValue(bond, 0.05, D), investmentValue(bond, 0.06, D), investmentValue(bond, 0.07, D)}) << "\n";
    const std::vector<std::pair<const char*, Bond>> named = {
        {"5y 4%", couponBond(100.0, 4.0, 5)}, {"10y 8%", couponBond(100.0, 8.0, 10)}, {"10y zero", zeroCoupon(100.0, 10.0)}};
    for (const auto& [name, b] : named)
        os << std::format("{}: price={} D={} at y=0.06; D at y=0.08: {}\n", name, f(price(b, y0)), f(duration(b, y0)),
                          f(duration(b, 0.08)));

    os << "\n== 10.1.3 Portfolios of bonds: A = 5y 4%, B = 10y 8%, horizon 5 years, y=0.06 ==\n";
    const Bond A = couponBond(100.0, 4.0, 5);
    const Bond B = couponBond(100.0, 8.0, 10);
    const double DA = duration(A, y0), DB = duration(B, y0);
    const auto [wA, wB] = immunisingWeights(DA, DB, 5.0);
    os << std::format("D_A={} D_B={}: weights w_A={} w_B={} give D={}\n", f(DA), f(DB), f(wA), f(wB), f(wA * DA + wB * DB));
    const double V0 = 100000.0;
    const Portfolio p = immunise(V0, A, B, y0, 0.0, 5.0);
    os << std::format("Invest {}: {} of A and {} of B\n", f(V0), f(p.q1), f(p.q2));
    const double target = V0 * std::exp(y0 * 5.0);
    os << std::format("Target at t=5 if y stays 0.06: {}\n", f(target));
    os << "Value at t=5 after an immediate jump of y (coupons reinvested at the new yield):\n";
    for (double y : {0.04, 0.05, 0.06, 0.07, 0.08}) {
        const double v = p.q1 * investmentValue(A, y, 5.0) + p.q2 * investmentValue(B, y, 5.0);
        os << std::format("y={}: portfolio {} (shortfall {}), B alone {}, A alone {}\n", f(y), f(v), f(target - v),
                          f(V0 / price(B, y0) * investmentValue(B, y, 5.0)), f(V0 / price(A, y0) * investmentValue(A, y, 5.0)));
    }

    os << "\n== 10.1.4 Dynamic hedging: re-immunise every half year along a yield path ==\n";
    const std::vector<std::pair<double, double>> path = {{0.0, 0.06}, {0.5, 0.065}, {1.0, 0.07}, {1.5, 0.06},
                                                         {2.0, 0.05}, {2.5, 0.055}, {3.0, 0.065}, {3.5, 0.07},
                                                         {4.0, 0.075}, {4.5, 0.07}, {5.0, 0.065}};
    for (const HedgeStep& st : dynamicHedge(V0, A, B, 5.0, path))
        os << std::format("t={} y={}: value={} q_A={} q_B={} D={}\n", f(st.t), f(st.y), f(st.value), f(st.q1), f(st.q2),
                          f(st.duration));
    const auto stat = dynamicHedge(V0, A, B, 5.0, path, false);
    os << std::format("At t=5: rebalanced {}, not rebalanced {}, target {}\n", f(dynamicHedge(V0, A, B, 5.0, path).back().value),
                      f(stat.back().value), f(target));

    os << "\n== 10.2.1 Forward rates: B(0,T) for T=0.5..3 ==\n";
    const std::vector<double> Ts = {0.5, 1.0, 1.5, 2.0, 2.5, 3.0};
    const std::vector<double> Bs = {0.978, 0.953, 0.926, 0.898, 0.870, 0.842};
    const std::vector<double> fw = forwardRates(Ts, Bs);
    for (std::size_t i = 0; i < Ts.size(); ++i)
        os << std::format("T={}: B={} y(0,T)={} f(0,T-0.5,T)={}\n", f(Ts[i]), f(Bs[i]), f(yieldRate(Bs[i], Ts[i])), f(fw[i]));
    os << std::format("Prices rebuilt from the forward rates: {}\n", fl(pricesFromForwardRates(Ts, fw)));
    double avg = 0.0;
    for (std::size_t i = 2; i < 6; ++i) avg += fw[i];
    os << std::format("f(0,1,3)={} = average of the half-year forwards {}\n", f(forwardRate(Bs[1], 1.0, Bs[5], 3.0)), f(avg / 4));
    std::string row = "Implied B(1,T) = B(0,T)/B(0,1):";
    for (std::size_t i = 2; i < Ts.size(); ++i) row += std::format(" T={}:{}", f(Ts[i]), f(impliedPrice(Bs[1], Bs[i])));
    os << row << "\n";
    os << std::format("FRA: borrow 1000 at t=1, repay at t=2 at f(0,1,2)={}: buy 1000 one-year bonds (cost {}), sell {} two-year bonds; repay {}\n",
                      f(forwardRate(Bs[1], 1.0, Bs[3], 2.0)), f(1000.0 * Bs[1]), f(1000.0 * Bs[1] / Bs[3]),
                      f(1000.0 * std::exp(forwardRate(Bs[1], 1.0, Bs[3], 2.0))));
    const std::vector<double> ft = {1.0, 2.0, 3.0};
    const std::vector<double> fC = {6.0, 6.0, 106.0};
    const double P = couponBondPrice(ft, fC, Ts, Bs);
    os << std::format("3-year 6% coupon bond under the term structure: price {}, yield to maturity {} (between y(0,1)={} and y(0,3)={})\n",
                      f(P), f(yieldToMaturity(P, ft, fC)), f(yieldRate(Bs[1], 1.0)), f(yieldRate(Bs[5], 3.0)));

    os << "\n== 10.2.2 Money market account: rolling over half-year bonds at the forward rates ==\n";
    const std::vector<double> As = moneyMarketAccount(Ts, Bs);
    for (std::size_t i = 0; i < Ts.size(); ++i) os << std::format("A({})={} = 1/B(0,T)={}\n", f(Ts[i]), f(As[i]), f(1 / Bs[i]));
    double sumf = 0.0;
    for (double fr : fw) sumf += fr * 0.5;
    os << std::format("Short rates realised = forward rates: {}; A(3)=exp(sum f dt)={}\n", fl(fw), f(std::exp(sumf)));
    os << std::format("1 unit at t=0 in the 3-year bond: {} at t=3, same as the money market account {}\n", f(1 / Bs[5]), f(As[5]));
}

}  // namespace m4f::ch10
