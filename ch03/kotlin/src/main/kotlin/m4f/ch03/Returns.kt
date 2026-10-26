package m4f.ch03

import kotlin.math.ln

// Section 3.1, dynamics of stock prices: returns, logarithmic returns and their
// expectations over a finite set of scenarios.

// 3.1.1 Return
/** K(1) = (S(1) + div - S(0)) / S(0) */
fun oneStepReturn(S0: Double, S1: Double, dividend: Double = 0.0): Double = (S1 + dividend - S0) / S0

/** k(1) = ln(S(1) / S(0)) */
fun logReturn(S0: Double, S1: Double): Double = ln(S1 / S0)

/** One-step returns K(n) along a price path S(0), S(1), ... */
fun returns(path: List<Double>): List<Double> = (1 until path.size).map { oneStepReturn(path[it - 1], path[it]) }

fun logReturns(path: List<Double>): List<Double> = (1 until path.size).map { logReturn(path[it - 1], path[it]) }

/** K(0, n) from one-step returns: 1 + K(0, n) = (1 + K(1)) ... (1 + K(n)) */
fun compoundReturn(Ks: List<Double>): Double {
    var g = 1.0
    for (K in Ks) g *= 1 + K
    return g - 1
}

// 3.1.2 Expected return
fun expectation(values: List<Double>, probs: List<Double>): Double {
    var s = 0.0
    for (i in values.indices) s += values[i] * probs[i]
    return s
}

fun variance(values: List<Double>, probs: List<Double>): Double {
    val mu = expectation(values, probs)
    var s = 0.0
    for (i in values.indices) {
        val d = values[i] - mu
        s += probs[i] * d * d
    }
    return s
}
