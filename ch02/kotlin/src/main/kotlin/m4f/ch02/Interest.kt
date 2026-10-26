// Mathematics for Finance (Capinski & Zastawniak), Chapter 2: Risk-Free Assets.
// Section 2.1, time value of money. P is the principal (present value), r the
// annual interest rate, t the time in years, m the compounding periods per year.
package m4f.ch02

import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.pow

// 2.1.1 Simple interest: V(t) = (1 + t r) P
fun simpleValue(P: Double, r: Double, t: Double) = (1 + t * r) * P
fun simplePresentValue(V: Double, r: Double, t: Double) = V / (1 + t * r)
// Rate r such that P grows to V in time t under simple interest.
fun simpleRate(P: Double, V: Double, t: Double) = (V / P - 1) / t

// 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P
fun periodicValue(P: Double, r: Double, m: Int, t: Double) = (1 + r / m).pow(t * m) * P
fun periodicPresentValue(V: Double, r: Double, m: Int, t: Double) = V * (1 + r / m).pow(-t * m)
// Effective annual rate r_e = (1 + r/m)^m - 1
fun effectiveRate(r: Double, m: Int) = (1 + r / m).pow(m) - 1

// 2.1.3 Streams of payments: n payments C at the end of each period, per-period
// rate i (for annual payments with annual compounding, i = r).
// PA(i, n) = (1 - (1+i)^-n) / i, the present value of n unit payments.
fun annuityFactor(i: Double, n: Int) = (1 - (1 + i).pow(-n)) / i
fun annuityPresentValue(C: Double, i: Double, n: Int) = C * annuityFactor(i, n)
fun annuityFutureValue(C: Double, i: Double, n: Int) = C * ((1 + i).pow(n) - 1) / i
fun perpetuityPresentValue(C: Double, i: Double) = C / i
// Payment C repaying a loan PA in n installments: C = PA / annuityFactor.
fun loanInstallment(PA: Double, i: Double, n: Int) = PA / annuityFactor(i, n)

// 2.1.4 Continuous compounding: V(t) = e^(t r) P
fun continuousValue(P: Double, r: Double, t: Double) = exp(t * r) * P
fun continuousPresentValue(V: Double, r: Double, t: Double) = V * exp(-t * r)
fun continuousEffectiveRate(r: Double) = exp(r) - 1

// 2.1.5 Comparing compounding methods: rates giving the same growth.
fun continuousFromPeriodic(rm: Double, m: Int) = m * ln(1 + rm / m)
fun periodicFromContinuous(rc: Double, m: Int) = m * (exp(rc / m) - 1)
// Rate compounded k times a year equivalent to rm compounded m times a year.
fun periodicFromPeriodic(rm: Double, m: Int, k: Int) = k * ((1 + rm / m).pow(m.toDouble() / k) - 1)
