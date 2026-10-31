// Section 3.2, the binomial tree model: S(n) = S(0) (1+u)^k (1+d)^(n-k) after k up moves in n
// steps, with the money market growing at r per step.
package m4f.ch03

import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.ln1p
import kotlin.math.pow

/**
 * C(n, k) p^k (1-p)^(n-k), computed in log space: C(n, k) alone overflows a Double near
 * n = 1030 even when the probability itself is tiny. log C(n, k) is a sum of min(k, n-k)
 * terms, using C(n, k) = C(n, n-k).
 */
fun binomialProb(n: Int, k: Int, p: Double): Double {
    if (k < 0 || k > n) return 0.0
    if (p <= 0) return if (k == 0) 1.0 else 0.0
    if (p >= 1) return if (k == n) 1.0 else 0.0
    var logC = 0.0
    for (i in 0 until minOf(k, n - k)) {
        logC += ln((n - i).toDouble()) - ln((i + 1).toDouble())
    }
    return exp(logC + k * ln(p) + (n - k) * ln1p(-p))
}

/** Stock S(0) = [s0] moving by [u] or [d] each step, money market at [r] per step. */
data class BinomialTree(val s0: Double, val u: Double, val d: Double, val r: Double) {
    /** S(n) after k up moves out of n. */
    fun price(n: Int, k: Int): Double = s0 * (1 + u).pow(k) * (1 + d).pow(n - k)

    /** E(K(n)) = p u + (1-p) d, the same for every step. */
    fun expectedReturn(p: Double): Double = p * u + (1 - p) * d

    /**
     * E(S(n)) as a sum over the n+1 nodes at step n, O(n). Each term
     * C(n, k) p^k (1-p)^(n-k) S(n, k) is built in log space, updating log C(n, k) from k to
     * k+1, so nothing overflows for large n.
     */
    fun expectedPrice(n: Int, p: Double): Double {
        if (p <= 0) return price(n, 0)
        if (p >= 1) return price(n, n)
        val logUp = ln(p) + ln1p(u)
        val logDown = ln1p(-p) + ln1p(d)
        var logC = 0.0
        var sum = 0.0
        for (k in 0..n) {
            sum += exp(logC + k * logUp + (n - k) * logDown)
            if (k < n) logC += ln((n - k).toDouble()) - ln(k + 1.0)
        }
        return s0 * sum
    }

    /** 3.2.1 Risk-neutral probability exists exactly when d < r < u. */
    fun isArbitrageFree(): Boolean = d < r && r < u

    /** p* = (r - d) / (u - d), so that E*(K(n)) = r. */
    fun riskNeutralProb(): Double = (r - d) / (u - d)

    /** 3.2.2 Martingale property: S~(n) = (1+r)^-n S(n). */
    fun discountedPrice(n: Int, k: Int): Double = price(n, k) * (1 + r).pow(-n)

    /** E*(S~(n+1) | S(n) at node (n, k)); equals S~(n) under p*. */
    fun conditionalDiscounted(n: Int, k: Int): Double {
        val q = riskNeutralProb()
        return q * discountedPrice(n + 1, k + 1) + (1 - q) * discountedPrice(n + 1, k)
    }
}
