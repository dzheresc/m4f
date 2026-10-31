// Section 3.2, the binomial tree model: S(n) = S(0) (1+u)^k (1+d)^(n-k) after k up moves in n
// steps, with the money market growing at r per step.
package m4f.ch03

import kotlin.math.pow

/** C(n, k) p^k (1-p)^(n-k). */
fun binomialProb(n: Int, k: Int, p: Double): Double {
    var c = 1.0
    for (i in 0 until k) {
        c = c * (n - i) / (i + 1)
    }
    return c * p.pow(k) * (1 - p).pow(n - k)
}

/** Stock S(0) = [s0] moving by [u] or [d] each step, money market at [r] per step. */
data class BinomialTree(val s0: Double, val u: Double, val d: Double, val r: Double) {
    /** S(n) after k up moves out of n. */
    fun price(n: Int, k: Int): Double = s0 * (1 + u).pow(k) * (1 + d).pow(n - k)

    /** E(K(n)) = p u + (1-p) d, the same for every step. */
    fun expectedReturn(p: Double): Double = p * u + (1 - p) * d

    /** E(S(n)) as a sum over the n+1 nodes at step n. */
    fun expectedPrice(n: Int, p: Double): Double {
        var sum = 0.0
        for (k in 0..n) {
            sum += price(n, k) * binomialProb(n, k, p)
        }
        return sum
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
