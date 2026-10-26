package m4f.ch07

import kotlin.math.exp
import kotlin.math.max
import kotlin.math.pow
import kotlin.math.sqrt

/** Option prices used to illustrate Sections 7.3-7.5: the Cox-Ross-Rubinstein
 *  tree with N steps of length tau = T/N, u = e^(sigma sqrt(tau)), d = 1/u,
 *  growth e^(r tau) per step (Chapter 8 develops the model itself). */
fun crrPrice(
    S0: Double, X: Double, r: Double, sigma: Double, T: Double, N: Int, isCall: Boolean, american: Boolean = false,
): Double {
    val tau = T / N
    val up = exp(sigma * sqrt(tau))
    val down = 1 / up
    val growth = exp(r * tau)
    val q = (growth - down) / (up - down)
    fun payoff(S: Double) = if (isCall) max(S - X, 0.0) else max(X - S, 0.0)

    val values = DoubleArray(N + 1) { k -> payoff(S0 * up.pow(k) * down.pow(N - k)) }
    for (n in N - 1 downTo 0)
        for (k in 0..n) {
            var v = (q * values[k + 1] + (1 - q) * values[k]) / growth
            if (american) v = max(v, payoff(S0 * up.pow(k) * down.pow(n - k)))
            values[k] = v
        }
    return values[0]
}
