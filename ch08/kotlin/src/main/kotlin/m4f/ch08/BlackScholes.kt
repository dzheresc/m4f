// Section 8.3: the Black-Scholes formula, and the Cox-Ross-Rubinstein tree that converges to it
// (1+u = e^(sigma sqrt(tau)), 1+d = e^(-sigma sqrt(tau)), 1+r = e^(r tau), tau = T/N). r is the
// continuously compounded rate, `s` the stock price now, `strike` the strike X, `sigma` the
// volatility and `maturity` the time T to expiry in years.
package m4f.ch08

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.max
import kotlin.math.sqrt

/**
 * Standard normal CDF to double precision (Hart 1968, as given by G. West, "Better
 * approximations to cumulative normal functions").
 */
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

/** C = S N(d1) - X e^(-rT) N(d2). */
fun bsCall(s: Double, strike: Double, r: Double, sigma: Double, maturity: Double): Double {
    val (d1, d2) = d1d2(s, strike, r, sigma, maturity)
    return s * normCdf(d1) - strike * exp(-r * maturity) * normCdf(d2)
}

/** P = X e^(-rT) N(-d2) - S N(-d1). */
fun bsPut(s: Double, strike: Double, r: Double, sigma: Double, maturity: Double): Double {
    val (d1, d2) = d1d2(s, strike, r, sigma, maturity)
    return strike * exp(-r * maturity) * normCdf(-d2) - s * normCdf(-d1)
}

/** Shares in the replicating portfolio: N(d1). */
fun bsCallDelta(s: Double, strike: Double, r: Double, sigma: Double, maturity: Double): Double =
    normCdf(d1d2(s, strike, r, sigma, maturity).first)

/** Price in the CRR tree with [numSteps] steps up to [maturity]. */
fun crrTreePrice(
    s: Double,
    strike: Double,
    r: Double,
    sigma: Double,
    maturity: Double,
    numSteps: Int,
    isCall: Boolean,
    american: Boolean = false,
): Double {
    val tau = maturity / numSteps
    val u = exp(sigma * sqrt(tau)) - 1
    val d = exp(-sigma * sqrt(tau)) - 1
    val rStep = exp(r * tau) - 1
    // The same backward induction as optionTree, keeping one row instead of the whole tree and
    // calling the payoff directly.
    val q = riskNeutralProb(u, d, rStep)
    val price = StockPrices(s, u, d, numSteps)
    fun payoff(x: Double) = if (isCall) max(x - strike, 0.0) else max(strike - x, 0.0)
    val values = DoubleArray(numSteps + 1) { k -> payoff(price(numSteps, k)) }
    for (n in numSteps - 1 downTo 0) {
        for (k in 0..n) {
            var v = (q * values[k + 1] + (1 - q) * values[k]) / (1 + rStep)
            if (american) v = max(v, payoff(price(n, k)))
            values[k] = v
        }
    }
    return values[0]
}
