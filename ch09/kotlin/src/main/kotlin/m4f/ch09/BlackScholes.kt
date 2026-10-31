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

// Acklam's rational approximation to the normal quantile: central region numerator (A) and
// denominator (B), tails (C, D).
private val ACKLAM_A = doubleArrayOf(
    -3.969683028665376e+01, 2.209460984245205e+02, -2.759285104469687e+02,
    1.383577518672690e+02, -3.066479806614716e+01, 2.506628277459239e+00,
)
private val ACKLAM_B = doubleArrayOf(
    -5.447609879822406e+01, 1.615858368580409e+02, -1.556989798598866e+02,
    6.680131188771972e+01, -1.328068155288572e+01,
)
private val ACKLAM_C = doubleArrayOf(
    -7.784894002430293e-03, -3.223964580411365e-01, -2.400758277161838e+00,
    -2.549732539343734e+00, 4.374664141464968e+00, 2.938163982698783e+00,
)
private val ACKLAM_D = doubleArrayOf(
    7.784695709041462e-03, 3.224671290700398e-01, 2.445134137142996e+00, 3.754408661907416e+00,
)
private const val ACKLAM_LOW = 0.02425

/**
 * Inverse of [normCdf]. Acklam's rational approximation (relative error below 1.2e-9) followed
 * by one Halley step against normCdf itself, which brings it to full double precision, about
 * the cost of two normCdf calls. [p] outside (0, 1) returns -40 or 40.
 */
fun normInv(p: Double): Double {
    if (!(p > 0)) return -40.0
    if (!(p < 1)) return 40.0
    val a = ACKLAM_A
    val b = ACKLAM_B
    val c = ACKLAM_C
    val d = ACKLAM_D
    var x: Double
    if (p < ACKLAM_LOW || p > 1 - ACKLAM_LOW) {
        // Tails: q = sqrt(-2 ln(min(p, 1-p))).
        val q = sqrt(-2 * ln(if (p < ACKLAM_LOW) p else 1 - p))
        x = (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
            ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1)
        if (p > 1 - ACKLAM_LOW) x = -x
    } else {
        val q = p - 0.5
        val r = q * q
        x = (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
            (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1)
    }
    // Halley step on normCdf(x) - p.
    val e = normCdf(x) - p
    val u = e * 2.506628274631000502 * exp(x * x / 2)
    return x - u / (1 + x * u / 2)
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
    val theta0 = -s * n1 * sigma / (2 * sqrt(maturity))
    // Each normCdf is evaluated once.
    return if (isCall) {
        val nd1 = normCdf(d1)
        val nd2 = normCdf(d2)
        Greeks(
            price = s * nd1 - disc * nd2,
            delta = nd1,
            gamma = gamma,
            theta = theta0 - r * disc * nd2,
            vega = vega,
            rho = maturity * disc * nd2,
        )
    } else {
        val nmd1 = normCdf(-d1)
        val nmd2 = normCdf(-d2)
        Greeks(
            price = disc * nmd2 - s * nmd1,
            delta = normCdf(d1) - 1,
            gamma = gamma,
            theta = theta0 + r * disc * nmd2,
            vega = vega,
            rho = -maturity * disc * nmd2,
        )
    }
}
