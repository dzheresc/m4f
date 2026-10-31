#include "m4f/report.h"

#include <cmath>
#include <format>
#include <ostream>
#include <string>

#include "m4f/interest.h"
#include "m4f/money_market.h"

namespace m4f::ch02 {
namespace {

// Four decimals, round-half-even on the exact double. Adding 0.0 turns -0.0
// into 0.0 so that no "-0.0000" appears.
std::string Fmt(double v) { return std::format("{:.4f}", v + 0.0); }

void PrintSimpleInterest(std::ostream& os) {
  os << "\n== 2.1.1 Simple interest: V(t) = (1 + t r) P ==\n";
  for (double t : {0.5, 1.0, 2.0}) {
    os << std::format("P=2000 r=0.08 t={}: V(t)={}\n", Fmt(t),
                      Fmt(SimpleValue(2000.0, 0.08, t)));
  }
  os << std::format("Present value of 1000 due at t=0.5, r=0.10: {}\n",
                    Fmt(SimplePresentValue(1000.0, 0.10, 0.5)));
  os << std::format("Rate turning 1000 into 1100 in 2 years: {}\n",
                    Fmt(SimpleRate(1000.0, 1100.0, 2.0)));
}

void PrintPeriodicCompounding(std::ostream& os) {
  os << "\n== 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P ==\n";
  for (int m : {1, 2, 4, 12, 365}) {
    os << std::format("P=1000 r=0.12 m={}: V(1)={} effective rate={}\n", m,
                      Fmt(PeriodicValue(1000.0, 0.12, m, 1.0)),
                      Fmt(EffectiveRate(0.12, m)));
  }
  os << std::format("Present value of 1000 due in 3 years, r=0.10 annual: {}\n",
                    Fmt(PeriodicPresentValue(1000.0, 0.10, 1, 3.0)));
}

void PrintStreamsOfPayments(std::ostream& os) {
  os << "\n== 2.1.3 Streams of payments ==\n";
  os << std::format("Annuity of 10 x 1000 at 10%: PV={} FV={}\n",
                    Fmt(AnnuityPresentValue(1000.0, 0.10, 10)),
                    Fmt(AnnuityFutureValue(1000.0, 0.10, 10)));
  os << std::format("Perpetuity of 100 a year at 5%: PV={}\n",
                    Fmt(PerpetuityPresentValue(100.0, 0.05)));
  const double installment = LoanInstallment(100000.0, 0.06 / 12, 300);
  os << std::format(
      "Loan 100000 over 25 years, monthly at 6%: installment={} "
      "balance after 10 years={}\n",
      Fmt(installment), Fmt(AnnuityPresentValue(installment, 0.06 / 12, 180)));
}

void PrintContinuousCompounding(std::ostream& os) {
  os << "\n== 2.1.4 Continuous compounding: V(t) = e^(t r) P ==\n";
  os << std::format("P=1000 r=0.12: V(1)={} effective rate={}\n",
                    Fmt(ContinuousValue(1000.0, 0.12, 1.0)),
                    Fmt(ContinuousEffectiveRate(0.12)));
  os << std::format(
      "Present value of 1000 due in 3 years, r=0.10 continuous: {}\n",
      Fmt(ContinuousPresentValue(1000.0, 0.10, 3.0)));
}

void PrintComparingMethods(std::ostream& os) {
  os << "\n== 2.1.5 Comparing compounding methods (r=0.10, one year) ==\n";
  os << std::format(
      "Growth factor: simple {}, m=1 {}, m=2 {}, m=12 {}, continuous {}\n",
      Fmt(SimpleValue(1.0, 0.10, 1.0)), Fmt(PeriodicValue(1.0, 0.10, 1, 1.0)),
      Fmt(PeriodicValue(1.0, 0.10, 2, 1.0)),
      Fmt(PeriodicValue(1.0, 0.10, 12, 1.0)),
      Fmt(ContinuousValue(1.0, 0.10, 1.0)));
  os << std::format("12% monthly = {} continuous\n",
                    Fmt(ContinuousFromPeriodic(0.12, 12)));
  os << std::format("10% continuous = {} annual = {} monthly\n",
                    Fmt(PeriodicFromContinuous(0.10, 1)),
                    Fmt(PeriodicFromContinuous(0.10, 12)));
  os << std::format("12% quarterly = {} monthly = {} annual\n",
                    Fmt(PeriodicFromPeriodic(0.12, 4, 12)),
                    Fmt(PeriodicFromPeriodic(0.12, 4, 1)));
}

void PrintZeroCouponBonds(std::ostream& os) {
  os << "\n== 2.2.1 Zero-coupon bonds ==\n";
  os << std::format(
      "B(0,1)=0.92 implies: annual {}, semi-annual {}, continuous {}\n",
      Fmt(ImpliedRate(0.92, 1.0, 1)), Fmt(ImpliedRate(0.92, 1.0, 2)),
      Fmt(ImpliedRate(0.92, 1.0)));
  os << std::format("Face 100, T=2 at 10%: annual {}, continuous {}\n",
                    Fmt(100.0 * ZeroCouponPrice(0.10, 2.0, 1)),
                    Fmt(100.0 * ZeroCouponPrice(0.10, 2.0)));
}

void PrintCouponBonds(std::ostream& os) {
  os << "\n== 2.2.2 Coupon bonds: F=100, annual coupon 10, 3 years ==\n";
  for (double y : {0.12, 0.10, 0.08}) {
    os << std::format("yield {}: price {}\n", Fmt(y),
                      Fmt(CouponBondPrice(100.0, 10.0, 3, y)));
  }
  os << std::format("price 95.0000: yield to maturity {}\n",
                    Fmt(YieldToMaturity(95.0, 100.0, 10.0, 3)));
}

void PrintMoneyMarketAccount(std::ostream& os) {
  os << "\n== 2.2.3 Money market account ==\n";
  os << std::format("A(0)=100 at 8% continuous: A(1)={}\n",
                    Fmt(MoneyMarketValue(100.0, 0.08, 1.0)));
  const double a1 = MoneyMarketRollover(100.0, {0.98, 0.985, 0.97, 0.99});
  os << std::format(
      "Rolling over 3-month zero-coupon bonds (B=0.98, 0.985, 0.97, 0.99): "
      "A(1)={} continuous rate {}\n",
      Fmt(a1), Fmt(std::log(a1 / 100.0)));
}

}  // namespace

void PrintReport(std::ostream& os) {
  os << "Chapter 2: Risk-Free Assets (illustrative numbers, book formulas)\n";
  PrintSimpleInterest(os);
  PrintPeriodicCompounding(os);
  PrintStreamsOfPayments(os);
  PrintContinuousCompounding(os);
  PrintComparingMethods(os);
  PrintZeroCouponBonds(os);
  PrintCouponBonds(os);
  PrintMoneyMarketAccount(os);
}

}  // namespace m4f::ch02
