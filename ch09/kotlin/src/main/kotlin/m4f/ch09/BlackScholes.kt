// Black-Scholes prices and the Greek parameters (Section 9.1.2).
// s stock price, strike X, r continuously compounded rate, sigma volatility, maturity T time to
// expiry. Same normCdf as Chapter 8 (Hart 1968 / G. West).
package m4f.ch09

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.max
import kotlin.math.sqrt

/** Standard normal CDF to double precision. */
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
            val numerator = e * b
            b = 8.83883476483184e-02 * a + 1.75566716318264
            b = b * a + 16.064177579207
            b = b * a + 86.7807322029461
            b = b * a + 296.564248779674
            b = b * a + 637.333633378831
            b = b * a + 793.826512519948
            b = b * a + 440.413735824752
            c = numerator / b
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

/** Standard normal density. */
fun normPdf(x: Double): Double = exp(-x * x / 2) / 2.506628274631000502

/** Quantile of the standard normal, by bisection on [normCdf] (0 < p < 1). */
fun normInv(p: Double): Double {
    var lo = -40.0
    var hi = 40.0
    repeat(200) {
        val mid = (lo + hi) / 2
        if (normCdf(mid) < p) lo = mid else hi = mid
    }
    return (lo + hi) / 2
}

/** (d1, d2) of the Black-Scholes formula. */
fun d1d2(
    s: Double,
    strike: Double,
    r: Double,
    sigma: Double,
    maturity: Double,
): Pair<Double, Double> {
    val d1 = (ln(s / strike) + (r + sigma * sigma / 2) * maturity) / (sigma * sqrt(maturity))
    return Pair(d1, d1 - sigma * sqrt(maturity))
}

/** Payoff of a call or put at expiry. */
fun payoff(s: Double, strike: Double, isCall: Boolean): Double =
    if (isCall) max(s - strike, 0.0) else max(strike - s, 0.0)

/** Black-Scholes price; the payoff when [maturity] <= 0. */
fun bsPrice(
    s: Double,
    strike: Double,
    r: Double,
    sigma: Double,
    maturity: Double,
    isCall: Boolean,
): Double {
    if (maturity <= 0) return payoff(s, strike, isCall)
    val (d1, d2) = d1d2(s, strike, r, sigma, maturity)
    return if (isCall) {
        s * normCdf(d1) - strike * exp(-r * maturity) * normCdf(d2)
    } else {
        strike * exp(-r * maturity) * normCdf(-d2) - s * normCdf(-d1)
    }
}

/**
 * Price and Greek parameters of an option.
 *
 * @property delta dV/dS
 * @property gamma d2V/dS2
 * @property theta dV/dt (per year)
 * @property vega dV/dsigma
 * @property rho dV/dr
 */
data class Greeks(
    val price: Double,
    val delta: Double,
    val gamma: Double,
    val theta: Double,
    val vega: Double,
    val rho: Double,
)

/** Selects one Greek parameter; [label] is its name in the report. */
enum class Greek(val label: String) {
    DELTA("delta"),
    GAMMA("gamma"),
    THETA("theta"),
    VEGA("vega"),
    RHO("rho"),
}

/** The Greek parameter [which]. */
fun Greeks.get(which: Greek): Double = when (which) {
    Greek.DELTA -> delta
    Greek.GAMMA -> gamma
    Greek.THETA -> theta
    Greek.VEGA -> vega
    Greek.RHO -> rho
}

/** The Greek parameters of a European option in the Black-Scholes model. */
fun bsGreeks(
    s: Double,
    strike: Double,
    r: Double,
    sigma: Double,
    maturity: Double,
    isCall: Boolean,
): Greeks {
    val (d1, d2) = d1d2(s, strike, r, sigma, maturity)
    val n1 = normPdf(d1)
    val disc = strike * exp(-r * maturity)
    val gamma = n1 / (s * sigma * sqrt(maturity))
    val vega = s * n1 * sqrt(maturity)
    return if (isCall) {
        Greeks(
            price = s * normCdf(d1) - disc * normCdf(d2),
            delta = normCdf(d1),
            gamma = gamma,
            theta = -s * n1 * sigma / (2 * sqrt(maturity)) - r * disc * normCdf(d2),
            vega = vega,
            rho = maturity * disc * normCdf(d2),
        )
    } else {
        Greeks(
            price = disc * normCdf(-d2) - s * normCdf(-d1),
            delta = normCdf(d1) - 1,
            gamma = gamma,
            theta = -s * n1 * sigma / (2 * sqrt(maturity)) + r * disc * normCdf(-d2),
            vega = vega,
            rho = -maturity * disc * normCdf(-d2),
        )
    }
}
