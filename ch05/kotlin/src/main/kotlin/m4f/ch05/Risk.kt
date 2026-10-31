// Sections 5.1-5.2: risk as the standard deviation of return, and portfolios of two securities
// with weights w1 + w2 = 1.
package m4f.ch05

import kotlin.math.max
import kotlin.math.sqrt

// Section 5.1: risk, from returns k_i in scenarios with probabilities p_i.

/** Expected return sum_i p_i k_i. */
fun mean(k: List<Double>, p: List<Double>): Double {
    var s = 0.0
    for (i in k.indices) {
        s += k[i] * p[i]
    }
    return s
}

/** Cov(K1, K2) = sum_i p_i (k1_i - mu1) (k2_i - mu2). */
fun covariance(k1: List<Double>, k2: List<Double>, p: List<Double>): Double {
    val m1 = mean(k1, p)
    val m2 = mean(k2, p)
    var s = 0.0
    for (i in k1.indices) {
        s += p[i] * (k1[i] - m1) * (k2[i] - m2)
    }
    return s
}

/** Standard deviation of the return [k]. */
fun stdDev(k: List<Double>, p: List<Double>): Double = sqrt(covariance(k, k, p))

/**
 * Correlation coefficient rho_12. One pass for the three second moments; same arithmetic as
 * covariance(k1, k2, p) / (stdDev(k1, p) * stdDev(k2, p)).
 */
fun correlation(k1: List<Double>, k2: List<Double>, p: List<Double>): Double {
    val m1 = mean(k1, p)
    val m2 = mean(k2, p)
    var cov = 0.0
    var var1 = 0.0
    var var2 = 0.0
    for (i in k1.indices) {
        cov += p[i] * (k1[i] - m1) * (k2[i] - m2)
        var1 += p[i] * (k1[i] - m1) * (k1[i] - m1)
        var2 += p[i] * (k2[i] - m2) * (k2[i] - m2)
    }
    return cov / (sqrt(var1) * sqrt(var2))
}

// Section 5.2.1: two securities, w1 in the first and w2 = 1 - w1 in the second.

/** mu_V = w1 mu1 + w2 mu2. */
fun twoAssetMean(w1: Double, mu1: Double, mu2: Double): Double = w1 * mu1 + (1 - w1) * mu2

/**
 * sigma_V^2 = w1^2 s1^2 + w2^2 s2^2 + 2 w1 w2 rho s1 s2; clamped at 0 against rounding when
 * |rho| = 1 makes the portfolio riskless.
 */
fun twoAssetStd(w1: Double, s1: Double, s2: Double, rho: Double): Double {
    val w2 = 1 - w1
    return sqrt(max(0.0, w1 * w1 * s1 * s1 + w2 * w2 * s2 * s2 + 2 * w1 * w2 * rho * s1 * s2))
}

/** w1 of the minimum variance portfolio: (s2^2 - rho s1 s2) / (s1^2 + s2^2 - 2 rho s1 s2). */
fun minVarianceWeight(s1: Double, s2: Double, rho: Double): Double =
    (s2 * s2 - rho * s1 * s2) / (s1 * s1 + s2 * s2 - 2 * rho * s1 * s2)
