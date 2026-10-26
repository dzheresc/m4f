package m4f.ch08

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.max
import kotlin.math.sqrt

// Section 8.3: the Black-Scholes formula, and the Cox-Ross-Rubinstein tree that
// converges to it (1+u = e^(sigma sqrt(tau)), 1+d = e^(-sigma sqrt(tau)),
// 1+r = e^(r tau), tau = T/N). r is the continuously compounded rate.

/** Standard normal CDF to double precision (Hart 1968, as given by G. West,
 *  "Better approximations to cumulative normal functions"). */
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

fun d1d2(S: Double, X: Double, r: Double, sigma: Double, T: Double): Pair<Double, Double> {
    val d1 = (ln(S / X) + (r + sigma * sigma / 2) * T) / (sigma * sqrt(T))
    return Pair(d1, d1 - sigma * sqrt(T))
}

/** C = S N(d1) - X e^(-rT) N(d2) */
fun bsCall(S: Double, X: Double, r: Double, sigma: Double, T: Double): Double {
    val (d1, d2) = d1d2(S, X, r, sigma, T)
    return S * normCdf(d1) - X * exp(-r * T) * normCdf(d2)
}

/** P = X e^(-rT) N(-d2) - S N(-d1) */
fun bsPut(S: Double, X: Double, r: Double, sigma: Double, T: Double): Double {
    val (d1, d2) = d1d2(S, X, r, sigma, T)
    return X * exp(-r * T) * normCdf(-d2) - S * normCdf(-d1)
}

/** Shares in the replicating portfolio: N(d1) */
fun bsCallDelta(S: Double, X: Double, r: Double, sigma: Double, T: Double): Double = normCdf(d1d2(S, X, r, sigma, T).first)

fun crrTreePrice(
    S: Double, X: Double, r: Double, sigma: Double, T: Double, N: Int, isCall: Boolean, american: Boolean = false,
): Double {
    val tau = T / N
    val u = exp(sigma * sqrt(tau)) - 1
    val d = exp(-sigma * sqrt(tau)) - 1
    val rr = exp(r * tau) - 1
    val payoff: Payoff = if (isCall) { s -> max(s - X, 0.0) } else { s -> max(X - s, 0.0) }
    return optionTree(S, u, d, rr, N, payoff, american)[0][0]
}
