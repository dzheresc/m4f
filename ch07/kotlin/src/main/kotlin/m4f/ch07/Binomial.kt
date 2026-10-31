package m4f.ch07

import kotlin.math.exp
import kotlin.math.max
import kotlin.math.pow
import kotlin.math.sqrt

/**
 * Option prices used to illustrate Sections 7.3-7.5: the Cox-Ross-Rubinstein tree with N steps
 * of length tau = T/N, u = e^(sigma sqrt(tau)), d = 1/u, growth e^(r tau) per step (Chapter 8
 * develops the model itself).
 */
fun crrPrice(
    s0: Double,
    strike: Double,
    r: Double,
    sigma: Double,
    maturity: Double,
    numSteps: Int,
    isCall: Boolean,
    american: Boolean = false,
): Double {
    val tau = maturity / numSteps
    val up = exp(sigma * sqrt(tau))
    val down = 1 / up
    val growth = exp(r * tau)
    val q = (growth - down) / (up - down)
    fun payoff(s: Double) = if (isCall) max(s - strike, 0.0) else max(strike - s, 0.0)

    // With down = 1/up the stock price at node (n, k) is s0 up^(2k - n), so all node prices come
    // from one table of 2N+1 powers instead of two pow calls per node.
    val prices = DoubleArray(2 * numSteps + 1) { s0 * up.pow(it - numSteps) }
    val values = DoubleArray(numSteps + 1) { k -> payoff(prices[2 * k]) }
    for (n in numSteps - 1 downTo 0) {
        for (k in 0..n) {
            var v = (q * values[k + 1] + (1 - q) * values[k]) / growth
            if (american) v = max(v, payoff(prices[2 * k - n + numSteps]))
            values[k] = v
        }
    }
    return values[0]
}
