// Mathematics for Finance (Capinski & Zastawniak), Chapter 2: Risk-Free Assets.
// Section 2.1, time value of money. P is the principal (present value), r the
// annual interest rate, t the time in years, m the compounding periods per year.
#pragma once

#include <cmath>

namespace m4f::ch02 {

// 2.1.1 Simple interest: V(t) = (1 + t r) P
inline double simpleValue(double P, double r, double t) { return (1 + t * r) * P; }
inline double simplePresentValue(double V, double r, double t) { return V / (1 + t * r); }
// Rate r such that P grows to V in time t under simple interest.
inline double simpleRate(double P, double V, double t) { return (V / P - 1) / t; }

// 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P
inline double periodicValue(double P, double r, int m, double t) {
    return std::pow(1 + r / m, t * m) * P;
}
inline double periodicPresentValue(double V, double r, int m, double t) {
    return V * std::pow(1 + r / m, -t * m);
}
// Effective annual rate r_e = (1 + r/m)^m - 1
inline double effectiveRate(double r, int m) { return std::pow(1 + r / m, m) - 1; }

// 2.1.3 Streams of payments: n payments C at the end of each period, per-period
// rate i (for annual payments with annual compounding, i = r).
// PA(i, n) = (1 - (1+i)^-n) / i, the present value of n unit payments.
inline double annuityFactor(double i, int n) { return (1 - std::pow(1 + i, -n)) / i; }
inline double annuityPresentValue(double C, double i, int n) { return C * annuityFactor(i, n); }
inline double annuityFutureValue(double C, double i, int n) {
    return C * (std::pow(1 + i, n) - 1) / i;
}
inline double perpetuityPresentValue(double C, double i) { return C / i; }
// Payment C repaying a loan PA in n installments: C = PA / annuityFactor.
inline double loanInstallment(double PA, double i, int n) { return PA / annuityFactor(i, n); }

// 2.1.4 Continuous compounding: V(t) = e^(t r) P
inline double continuousValue(double P, double r, double t) { return std::exp(t * r) * P; }
inline double continuousPresentValue(double V, double r, double t) { return V * std::exp(-t * r); }
inline double continuousEffectiveRate(double r) { return std::exp(r) - 1; }

// 2.1.5 Comparing compounding methods: rates giving the same growth.
inline double continuousFromPeriodic(double rm, int m) { return m * std::log(1 + rm / m); }
inline double periodicFromContinuous(double rc, int m) { return m * (std::exp(rc / m) - 1); }
// Rate compounded k times a year equivalent to rm compounded m times a year.
inline double periodicFromPeriodic(double rm, int m, int k) {
    return k * (std::pow(1 + rm / m, double(m) / k) - 1);
}

}  // namespace m4f::ch02
