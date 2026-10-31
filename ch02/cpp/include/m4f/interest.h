// Mathematics for Finance (Capinski & Zastawniak), Chapter 2: Risk-Free Assets.
// Section 2.1, time value of money.
//
// Book notation: P is the principal (`principal`), V the future value
// (`value`), r the annual interest rate, t the time in years, m the number of
// compounding periods per year.
#ifndef M4F_CH02_INTEREST_H_
#define M4F_CH02_INTEREST_H_

#include <cmath>

namespace m4f::ch02 {

// 2.1.1 Simple interest: V(t) = (1 + t r) P.
inline double SimpleValue(double principal, double r, double t) {
  return (1 + t * r) * principal;
}
inline double SimplePresentValue(double value, double r, double t) {
  return value / (1 + t * r);
}
// Rate r such that `principal` grows to `value` in time t under simple
// interest.
inline double SimpleRate(double principal, double value, double t) {
  return (value / principal - 1) / t;
}

// 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P.
inline double PeriodicValue(double principal, double r, int m, double t) {
  return std::pow(1 + r / m, t * m) * principal;
}
inline double PeriodicPresentValue(double value, double r, int m, double t) {
  return value * std::pow(1 + r / m, -t * m);
}
// Effective annual rate r_e = (1 + r/m)^m - 1.
inline double EffectiveRate(double r, int m) {
  return std::pow(1 + r / m, m) - 1;
}

// 2.1.3 Streams of payments: n payments C (`payment`) at the end of each
// period, per-period rate i (for annual payments with annual compounding,
// i = r).

// PA(i, n) = (1 - (1+i)^-n) / i, the present value of n unit payments.
inline double AnnuityFactor(double i, int n) {
  return (1 - std::pow(1 + i, -n)) / i;
}
inline double AnnuityPresentValue(double payment, double i, int n) {
  return payment * AnnuityFactor(i, n);
}
inline double AnnuityFutureValue(double payment, double i, int n) {
  return payment * (std::pow(1 + i, n) - 1) / i;
}
inline double PerpetuityPresentValue(double payment, double i) {
  return payment / i;
}
// Payment C repaying a loan PA in n installments: C = PA / PA(i, n).
inline double LoanInstallment(double loan, double i, int n) {
  return loan / AnnuityFactor(i, n);
}

// 2.1.4 Continuous compounding: V(t) = e^(t r) P.
inline double ContinuousValue(double principal, double r, double t) {
  return std::exp(t * r) * principal;
}
inline double ContinuousPresentValue(double value, double r, double t) {
  return value * std::exp(-t * r);
}
inline double ContinuousEffectiveRate(double r) { return std::exp(r) - 1; }

// 2.1.5 Comparing compounding methods: rates giving the same growth. r_m is a
// rate compounded m times a year, r_c a continuously compounded rate.
inline double ContinuousFromPeriodic(double r_m, int m) {
  return m * std::log(1 + r_m / m);
}
inline double PeriodicFromContinuous(double r_c, int m) {
  return m * (std::exp(r_c / m) - 1);
}
// Rate compounded k times a year equivalent to r_m compounded m times a year.
inline double PeriodicFromPeriodic(double r_m, int m, int k) {
  return k * (std::pow(1 + r_m / m, static_cast<double>(m) / k) - 1);
}

}  // namespace m4f::ch02

#endif  // M4F_CH02_INTEREST_H_
