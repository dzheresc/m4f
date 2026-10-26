package m4f.ch03

import kotlin.math.PI
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.min
import kotlin.math.pow
import kotlin.math.sqrt

// Section 3.3, other models: the trinomial tree and the continuous-time limit of
// the binomial model (log returns m tau +/- sigma sqrt(tau) with probability 1/2).

// 3.3.1 Trinomial tree model: returns u > m > d, money market r.
data class RiskNeutralProbs(val pu: Double, val pm: Double, val pd: Double)

data class TrinomialModel(val u: Double, val m: Double, val d: Double, val r: Double) {
    /** (p_u, p_m, p_d) with p_u u + p_m m + p_d d = r for a chosen p_m; the
     *  risk-neutral probability is not unique in this model. */
    fun riskNeutralProbs(pm: Double): RiskNeutralProbs {
        val pu = (r - pm * m - (1 - pm) * d) / (u - d)
        return RiskNeutralProbs(pu, pm, 1 - pm - pu)
    }

    /** p_u, p_d > 0 exactly when 0 < p_m < this bound */
    fun maxMiddleProb(): Double = min((r - d) / (m - d), (u - r) / (u - m))
}

// 3.3.2 Continuous-time limit
/** Standard normal CDF, Abramowitz & Stegun 26.2.17 (|error| < 7.5e-8) */
fun normCdf(x: Double): Double {
    if (x < 0) return 1 - normCdf(-x)
    val t = 1 / (1 + 0.2316419 * x)
    val poly = t * (0.319381530 + t * (-0.356563782 + t * (1.781477937 + t * (-1.821255978 + t * 1.330274429))))
    return 1 - exp(-x * x / 2) / sqrt(2 * PI) * poly
}

/** E(S(T)) with N steps of length tau = T/N */
fun binomialExpectedPrice(S0: Double, mu: Double, sigma: Double, T: Double, N: Int): Double {
    val tau = T / N
    val a = mu * tau
    val b = sigma * sqrt(tau)
    return S0 * (0.5 * exp(a + b) + 0.5 * exp(a - b)).pow(N)
}

/** Limit N -> oo: E(S(T)) = S(0) e^((mu + sigma^2/2) T) */
fun lognormalExpectedPrice(S0: Double, mu: Double, sigma: Double, T: Double): Double =
    S0 * exp((mu + sigma * sigma / 2) * T)

/** P(S(T) > K) in the N-step binomial model */
fun binomialProbAbove(S0: Double, mu: Double, sigma: Double, T: Double, N: Int, K: Double): Double {
    val b = sigma * sqrt(T / N)
    val threshold = ln(K / S0) - mu * T
    val logHalfN = N * ln(0.5)
    var logC = 0.0
    var total = 0.0
    for (k in 0..N) {
        if ((2 * k - N) * b > threshold) total += exp(logC + logHalfN)
        if (k < N) logC += ln((N - k).toDouble()) - ln((k + 1).toDouble())
    }
    return total
}

/** Limit N -> oo: ln S(T) ~ N(ln S(0) + mu T, sigma^2 T) */
fun lognormalProbAbove(S0: Double, mu: Double, sigma: Double, T: Double, K: Double): Double =
    normCdf((ln(S0 / K) + mu * T) / (sigma * sqrt(T)))
