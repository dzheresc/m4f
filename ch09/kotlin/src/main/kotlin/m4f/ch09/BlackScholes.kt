package m4f.ch09

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.max
import kotlin.math.sqrt

// Black-Scholes prices and the Greek parameters (Section 9.1.2).
// S stock price, X strike, r continuously compounded rate, sigma volatility,
// T time to expiry. Same normCdf as Chapter 8 (Hart 1968 / G. West).

fun normCdf(x: Double): Double {
    val a = abs(x)
    val c: Double
    if (a > 37.0) {
        c = 0.0
    } else {
        val e = exp(-a * a / 2)
        var b: Double
        if (a < 7.07106781186547) {
            b = 3.52624965998911e-02 * a + 0.700383064443688
            b = b * a + 6.37396220353165
            b = b * a + 33.912866078383
            b = b * a + 112.079291497871
            b = b * a + 221.213596169931
            b = b * a + 220.206867912376
            var cc = e * b
            b = 8.83883476483184e-02 * a + 1.75566716318264
            b = b * a + 16.064177579207
            b = b * a + 86.7807322029461
            b = b * a + 296.564248779674
            b = b * a + 637.333633378831
            b = b * a + 793.826512519948
            b = b * a + 440.413735824752
            cc = cc / b
            c = cc
        } else {
            b = a + 0.65
            b = a + 4 / b
            b = a + 3 / b
            b = a + 2 / b
            b = a + 1 / b
            c = e / b / 2.506628274631
        }
    }
    return if (x > 0) 1 - c else c
}

fun normPdf(x: Double): Double = exp(-x * x / 2) / 2.506628274631000502

/** Quantile of the standard normal, by bisection on normCdf (0 < p < 1). */
fun normInv(p: Double): Double {
    var lo = -40.0
    var hi = 40.0
    repeat(200) {
        val mid = (lo + hi) / 2
        if (normCdf(mid) < p) lo = mid else hi = mid
    }
    return (lo + hi) / 2
}

fun d1d2(S: Double, X: Double, r: Double, sigma: Double, T: Double): Pair<Double, Double> {
    val d1 = (ln(S / X) + (r + sigma * sigma / 2) * T) / (sigma * sqrt(T))
    return Pair(d1, d1 - sigma * sqrt(T))
}

fun payoff(S: Double, X: Double, isCall: Boolean): Double = if (isCall) max(S - X, 0.0) else max(X - S, 0.0)

/** Black-Scholes price; the payoff when T <= 0. */
fun bsPrice(S: Double, X: Double, r: Double, sigma: Double, T: Double, isCall: Boolean): Double {
    if (T <= 0) return payoff(S, X, isCall)
    val (d1, d2) = d1d2(S, X, r, sigma, T)
    return if (isCall) S * normCdf(d1) - X * exp(-r * T) * normCdf(d2)
    else X * exp(-r * T) * normCdf(-d2) - S * normCdf(-d1)
}

data class Greeks(
    val price: Double,
    val delta: Double,  // dV/dS
    val gamma: Double,  // d2V/dS2
    val theta: Double,  // dV/dt (per year)
    val vega: Double,   // dV/dsigma
    val rho: Double,    // dV/dr
)

enum class Greek(val label: String) { DELTA("delta"), GAMMA("gamma"), THETA("theta"), VEGA("vega"), RHO("rho") }

fun Greeks.get(which: Greek): Double = when (which) {
    Greek.DELTA -> delta
    Greek.GAMMA -> gamma
    Greek.THETA -> theta
    Greek.VEGA -> vega
    Greek.RHO -> rho
}

/** The Greek parameters of a European option in the Black-Scholes model. */
fun bsGreeks(S: Double, X: Double, r: Double, sigma: Double, T: Double, isCall: Boolean): Greeks {
    val (d1, d2) = d1d2(S, X, r, sigma, T)
    val n1 = normPdf(d1)
    val disc = X * exp(-r * T)
    val gamma = n1 / (S * sigma * sqrt(T))
    val vega = S * n1 * sqrt(T)
    return if (isCall) Greeks(
        S * normCdf(d1) - disc * normCdf(d2), normCdf(d1), gamma,
        -S * n1 * sigma / (2 * sqrt(T)) - r * disc * normCdf(d2), vega, T * disc * normCdf(d2),
    ) else Greeks(
        disc * normCdf(-d2) - S * normCdf(-d1), normCdf(d1) - 1, gamma,
        -S * n1 * sigma / (2 * sqrt(T)) + r * disc * normCdf(-d2), vega, -T * disc * normCdf(-d2),
    )
}
