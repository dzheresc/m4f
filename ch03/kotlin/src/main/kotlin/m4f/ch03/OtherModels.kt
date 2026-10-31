// Section 3.3, other models: the trinomial tree and the continuous-time limit of the binomial
// model (log returns m tau +/- sigma sqrt(tau) with probability 1/2). S(0) is s0, T the horizon,
// N the number of steps (numSteps), K a threshold price.
package m4f.ch03

import kotlin.math.PI
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.min
import kotlin.math.pow
import kotlin.math.sqrt

/** Risk-neutral probabilities (p_u, p_m, p_d) of the trinomial model. */
data class RiskNeutralProbs(val pUp: Double, val pMid: Double, val pDown: Double)

/** 3.3.1 Trinomial tree model: returns [u] > [m] > [d], money market [r]. */
data class TrinomialModel(val u: Double, val m: Double, val d: Double, val r: Double) {
    /**
     * (p_u, p_m, p_d) with p_u u + p_m m + p_d d = r for a chosen p_m; the risk-neutral
     * probability is not unique in this model.
     */
    fun riskNeutralProbs(pMid: Double): RiskNeutralProbs {
        val pUp = (r - pMid * m - (1 - pMid) * d) / (u - d)
        return RiskNeutralProbs(pUp, pMid, 1 - pMid - pUp)
    }

    /** p_u, p_d > 0 exactly when 0 < p_m < this bound. */
    fun maxMiddleProb(): Double = min((r - d) / (m - d), (u - r) / (u - m))
}

/** 3.3.2 Standard normal CDF, Abramowitz & Stegun 26.2.17 (|error| < 7.5e-8). */
fun normCdf(x: Double): Double {
    if (x < 0) return 1 - normCdf(-x)
    val t = 1 / (1 + 0.2316419 * x)
    val poly = t * (
        0.319381530 +
            t * (-0.356563782 + t * (1.781477937 + t * (-1.821255978 + t * 1.330274429)))
        )
    return 1 - exp(-x * x / 2) / sqrt(2 * PI) * poly
}

/** E(S(T)) with N steps of length tau = T/N. */
fun binomialExpectedPrice(
    s0: Double,
    mu: Double,
    sigma: Double,
    horizon: Double,
    numSteps: Int,
): Double {
    val tau = horizon / numSteps
    val a = mu * tau
    val b = sigma * sqrt(tau)
    return s0 * (0.5 * exp(a + b) + 0.5 * exp(a - b)).pow(numSteps)
}

/** Limit N -> oo: E(S(T)) = S(0) e^((mu + sigma^2/2) T). */
fun lognormalExpectedPrice(s0: Double, mu: Double, sigma: Double, horizon: Double): Double =
    s0 * exp((mu + sigma * sigma / 2) * horizon)

/** P(S(T) > K) in the N-step binomial model. */
fun binomialProbAbove(
    s0: Double,
    mu: Double,
    sigma: Double,
    horizon: Double,
    numSteps: Int,
    threshold: Double,
): Double {
    val b = sigma * sqrt(horizon / numSteps)
    val logThreshold = ln(threshold / s0) - mu * horizon
    val logHalfN = numSteps * ln(0.5)
    var logC = 0.0
    var total = 0.0
    for (k in 0..numSteps) {
        if ((2 * k - numSteps) * b > logThreshold) total += exp(logC + logHalfN)
        if (k < numSteps) logC += ln((numSteps - k).toDouble()) - ln((k + 1).toDouble())
    }
    return total
}

/** Limit N -> oo: ln S(T) ~ N(ln S(0) + mu T, sigma^2 T). */
fun lognormalProbAbove(
    s0: Double,
    mu: Double,
    sigma: Double,
    horizon: Double,
    threshold: Double,
): Double = normCdf((ln(s0 / threshold) + mu * horizon) / (sigma * sqrt(horizon)))
