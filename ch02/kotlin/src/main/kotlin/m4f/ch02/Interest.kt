// Mathematics for Finance (Capinski & Zastawniak), Chapter 2: Risk-Free Assets.
// Section 2.1, time value of money.
//
// Book notation: P is the principal (principal), V the future value (value), r the annual
// interest rate, t the time in years, m the number of compounding periods per year.
package m4f.ch02

import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.pow

// 2.1.1 Simple interest: V(t) = (1 + t r) P.

fun simpleValue(principal: Double, r: Double, t: Double) = (1 + t * r) * principal

fun simplePresentValue(value: Double, r: Double, t: Double) = value / (1 + t * r)

/** Rate r such that [principal] grows to [value] in time [t] under simple interest. */
fun simpleRate(principal: Double, value: Double, t: Double) = (value / principal - 1) / t

// 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P.

fun periodicValue(principal: Double, r: Double, m: Int, t: Double) =
    (1 + r / m).pow(t * m) * principal

fun periodicPresentValue(value: Double, r: Double, m: Int, t: Double) =
    value * (1 + r / m).pow(-t * m)

/** Effective annual rate r_e = (1 + r/m)^m - 1. */
fun effectiveRate(r: Double, m: Int) = (1 + r / m).pow(m) - 1

// 2.1.3 Streams of payments: n payments C (payment) at the end of each period, per-period
// rate i (for annual payments with annual compounding, i = r).

/** PA(i, n) = (1 - (1+i)^-n) / i, the present value of n unit payments. */
fun annuityFactor(i: Double, n: Int) = (1 - (1 + i).pow(-n)) / i

fun annuityPresentValue(payment: Double, i: Double, n: Int) = payment * annuityFactor(i, n)

fun annuityFutureValue(payment: Double, i: Double, n: Int) = payment * ((1 + i).pow(n) - 1) / i

fun perpetuityPresentValue(payment: Double, i: Double) = payment / i

/** Payment C repaying a loan PA in n installments: C = PA / PA(i, n). */
fun loanInstallment(loan: Double, i: Double, n: Int) = loan / annuityFactor(i, n)

// 2.1.4 Continuous compounding: V(t) = e^(t r) P.

fun continuousValue(principal: Double, r: Double, t: Double) = exp(t * r) * principal

fun continuousPresentValue(value: Double, r: Double, t: Double) = value * exp(-t * r)

fun continuousEffectiveRate(r: Double) = exp(r) - 1

// 2.1.5 Comparing compounding methods: rates giving the same growth. rM is a rate compounded
// m times a year, rC a continuously compounded rate.

fun continuousFromPeriodic(rM: Double, m: Int) = m * ln(1 + rM / m)

fun periodicFromContinuous(rC: Double, m: Int) = m * (exp(rC / m) - 1)

/** Rate compounded [k] times a year equivalent to [rM] compounded [m] times a year. */
fun periodicFromPeriodic(rM: Double, m: Int, k: Int) = k * ((1 + rM / m).pow(m.toDouble() / k) - 1)
