#include "m4f/report.hpp"
#include "m4f/interest.hpp"
#include "m4f/money_market.hpp"

#include <cmath>
#include <format>
#include <ostream>

namespace m4f::ch02 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

}  // namespace

void printReport(std::ostream& os) {
    os << "Chapter 2: Risk-Free Assets (illustrative numbers, book formulas)\n";

    os << "\n== 2.1.1 Simple interest: V(t) = (1 + t r) P ==\n";
    for (double t : {0.5, 1.0, 2.0})
        os << std::format("P=2000 r=0.08 t={}: V(t)={}\n", f(t), f(simpleValue(2000.0, 0.08, t)));
    os << std::format("Present value of 1000 due at t=0.5, r=0.10: {}\n",
                      f(simplePresentValue(1000.0, 0.10, 0.5)));
    os << std::format("Rate turning 1000 into 1100 in 2 years: {}\n",
                      f(simpleRate(1000.0, 1100.0, 2.0)));

    os << "\n== 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P ==\n";
    for (int m : {1, 2, 4, 12, 365})
        os << std::format("P=1000 r=0.12 m={}: V(1)={} effective rate={}\n", m,
                          f(periodicValue(1000.0, 0.12, m, 1.0)), f(effectiveRate(0.12, m)));
    os << std::format("Present value of 1000 due in 3 years, r=0.10 annual: {}\n",
                      f(periodicPresentValue(1000.0, 0.10, 1, 3.0)));

    os << "\n== 2.1.3 Streams of payments ==\n";
    os << std::format("Annuity of 10 x 1000 at 10%: PV={} FV={}\n",
                      f(annuityPresentValue(1000.0, 0.10, 10)),
                      f(annuityFutureValue(1000.0, 0.10, 10)));
    os << std::format("Perpetuity of 100 a year at 5%: PV={}\n",
                      f(perpetuityPresentValue(100.0, 0.05)));
    const double C = loanInstallment(100000.0, 0.06 / 12, 300);
    os << std::format("Loan 100000 over 25 years, monthly at 6%: installment={} "
                      "balance after 10 years={}\n",
                      f(C), f(annuityPresentValue(C, 0.06 / 12, 180)));

    os << "\n== 2.1.4 Continuous compounding: V(t) = e^(t r) P ==\n";
    os << std::format("P=1000 r=0.12: V(1)={} effective rate={}\n",
                      f(continuousValue(1000.0, 0.12, 1.0)), f(continuousEffectiveRate(0.12)));
    os << std::format("Present value of 1000 due in 3 years, r=0.10 continuous: {}\n",
                      f(continuousPresentValue(1000.0, 0.10, 3.0)));

    os << "\n== 2.1.5 Comparing compounding methods (r=0.10, one year) ==\n";
    os << std::format("Growth factor: simple {}, m=1 {}, m=2 {}, m=12 {}, continuous {}\n",
                      f(simpleValue(1.0, 0.10, 1.0)), f(periodicValue(1.0, 0.10, 1, 1.0)),
                      f(periodicValue(1.0, 0.10, 2, 1.0)), f(periodicValue(1.0, 0.10, 12, 1.0)),
                      f(continuousValue(1.0, 0.10, 1.0)));
    os << std::format("12% monthly = {} continuous\n", f(continuousFromPeriodic(0.12, 12)));
    os << std::format("10% continuous = {} annual = {} monthly\n",
                      f(periodicFromContinuous(0.10, 1)), f(periodicFromContinuous(0.10, 12)));
    os << std::format("12% quarterly = {} monthly = {} annual\n",
                      f(periodicFromPeriodic(0.12, 4, 12)), f(periodicFromPeriodic(0.12, 4, 1)));

    os << "\n== 2.2.1 Zero-coupon bonds ==\n";
    os << std::format("B(0,1)=0.92 implies: annual {}, semi-annual {}, continuous {}\n",
                      f(impliedRate(0.92, 1.0, 1)), f(impliedRate(0.92, 1.0, 2)),
                      f(impliedRate(0.92, 1.0)));
    os << std::format("Face 100, T=2 at 10%: annual {}, continuous {}\n",
                      f(100.0 * zeroCouponPrice(0.10, 2.0, 1)), f(100.0 * zeroCouponPrice(0.10, 2.0)));

    os << "\n== 2.2.2 Coupon bonds: F=100, annual coupon 10, 3 years ==\n";
    for (double y : {0.12, 0.10, 0.08})
        os << std::format("yield {}: price {}\n", f(y), f(couponBondPrice(100.0, 10.0, 3, y)));
    os << std::format("price 95.0000: yield to maturity {}\n",
                      f(yieldToMaturity(95.0, 100.0, 10.0, 3)));

    os << "\n== 2.2.3 Money market account ==\n";
    os << std::format("A(0)=100 at 8% continuous: A(1)={}\n", f(moneyMarketValue(100.0, 0.08, 1.0)));
    const double A1 = moneyMarketRollover(100.0, {0.98, 0.985, 0.97, 0.99});
    os << std::format("Rolling over 3-month zero-coupon bonds (B=0.98, 0.985, 0.97, 0.99): A(1)={} "
                      "continuous rate {}\n",
                      f(A1), f(std::log(A1 / 100.0)));
}

}  // namespace m4f::ch02
