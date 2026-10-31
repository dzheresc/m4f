// Section 3.1, dynamics of stock prices: returns, logarithmic returns and their expectations
// over a finite set of scenarios. S(0), S(1) are s0, s1; K is a one-step return and k a
// logarithmic return.
package m4f.ch03

import kotlin.math.ln

/** 3.1.1 Return: K(1) = (S(1) + div - S(0)) / S(0). */
fun oneStepReturn(s0: Double, s1: Double, dividend: Double = 0.0): Double =
    (s1 + dividend - s0) / s0

/** Logarithmic return k(1) = ln(S(1) / S(0)). */
fun logReturn(s0: Double, s1: Double): Double = ln(s1 / s0)

/** One-step returns K(n) along a price path S(0), S(1), ... */
fun returns(path: List<Double>): List<Double> =
    (1 until path.size).map { oneStepReturn(path[it - 1], path[it]) }

/** One-step logarithmic returns k(n) along a price path. */
fun logReturns(path: List<Double>): List<Double> =
    (1 until path.size).map { logReturn(path[it - 1], path[it]) }

/** K(0, n) from one-step returns: 1 + K(0, n) = (1 + K(1)) ... (1 + K(n)). */
fun compoundReturn(oneStepReturns: List<Double>): Double {
    var growth = 1.0
    for (k in oneStepReturns) {
        growth *= 1 + k
    }
    return growth - 1
}

/** 3.1.2 Expected return: E(X) over a finite set of scenarios. */
fun expectation(values: List<Double>, probs: List<Double>): Double {
    var sum = 0.0
    for (i in values.indices) {
        sum += values[i] * probs[i]
    }
    return sum
}

fun variance(values: List<Double>, probs: List<Double>): Double {
    val mean = expectation(values, probs)
    var sum = 0.0
    for (i in values.indices) {
        val deviation = values[i] - mean
        sum += probs[i] * deviation * deviation
    }
    return sum
}
