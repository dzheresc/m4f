package m4f.ch05

import kotlin.math.max
import kotlin.math.sqrt

// Sections 5.1-5.2: risk as the standard deviation of return, and portfolios of
// two securities with weights w1 + w2 = 1.

// 5.1 Risk, from returns K_i in scenarios with probabilities p_i
fun mean(K: List<Double>, p: List<Double>): Double {
    var s = 0.0
    for (i in K.indices) s += K[i] * p[i]
    return s
}

fun covariance(K1: List<Double>, K2: List<Double>, p: List<Double>): Double {
    val m1 = mean(K1, p)
    val m2 = mean(K2, p)
    var s = 0.0
    for (i in K1.indices) s += p[i] * (K1[i] - m1) * (K2[i] - m2)
    return s
}

fun stdDev(K: List<Double>, p: List<Double>): Double = sqrt(covariance(K, K, p))

fun correlation(K1: List<Double>, K2: List<Double>, p: List<Double>): Double =
    covariance(K1, K2, p) / (stdDev(K1, p) * stdDev(K2, p))

// 5.2.1 Two securities: w1 in the first, w2 = 1 - w1 in the second
/** mu_V = w1 mu1 + w2 mu2 */
fun twoAssetMean(w1: Double, mu1: Double, mu2: Double): Double = w1 * mu1 + (1 - w1) * mu2

/** sigma_V^2 = w1^2 s1^2 + w2^2 s2^2 + 2 w1 w2 rho s1 s2; clamped at 0 against
 *  rounding when |rho| = 1 makes the portfolio riskless. */
fun twoAssetStd(w1: Double, s1: Double, s2: Double, rho: Double): Double {
    val w2 = 1 - w1
    return sqrt(max(0.0, w1 * w1 * s1 * s1 + w2 * w2 * s2 * s2 + 2 * w1 * w2 * rho * s1 * s2))
}

/** w1 of the minimum variance portfolio: (s2^2 - rho s1 s2) / (s1^2 + s2^2 - 2 rho s1 s2) */
fun minVarianceWeight(s1: Double, s2: Double, rho: Double): Double =
    (s2 * s2 - rho * s1 * s2) / (s1 * s1 + s2 * s2 - 2 * rho * s1 * s2)
