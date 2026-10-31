// Section 1.5: forward contracts. A portfolio (x, y, z) adds z forward contracts (z > 0 long,
// z < 0 short) with forward price F to x shares and y bonds; entering a forward costs nothing at
// t=0. The forward price F is called deliveryPrice below. Other names follow the book:
// s0 = S(0), s1 = S(1), a0 = A(0), a1 = A(1), r = the risk-free return over the step.
package m4f.ch01

/** [x] shares, [y] bonds and [z] forward contracts. */
data class ForwardPortfolio(val x: Double, val y: Double, val z: Double) {
    fun value0(s0: Double, a0: Double) = x * s0 + y * a0

    fun value1(s1: Double, a1: Double, deliveryPrice: Double) =
        x * s1 + y * a1 + z * (s1 - deliveryPrice)
}

fun longForwardPayoff(s1: Double, deliveryPrice: Double) = s1 - deliveryPrice

fun shortForwardPayoff(s1: Double, deliveryPrice: Double) = deliveryPrice - s1

/** Proposition 1.2: the no-arbitrage forward price F = S(0) A(1) / A(0) = S(0) (1 + r). */
fun forwardPrice(s0: Double, a0: Double, a1: Double) = s0 * a1 / a0

/**
 * Cash-and-carry arbitrage against a mispriced forward, one contract:
 * - F > fair: borrow S(0), buy a share, short the forward -> V(1) = F - fair;
 * - F < fair: short a share, invest S(0), long the forward -> V(1) = fair - F.
 *
 * V(0) = 0 and V(1) does not depend on S(1). Returns null if [deliveryPrice] is fair.
 */
fun forwardArbitrage(s0: Double, a0: Double, a1: Double, deliveryPrice: Double): ForwardPortfolio? {
    val fair = forwardPrice(s0, a0, a1)
    return when {
        deliveryPrice > fair -> ForwardPortfolio(1.0, -s0 / a0, -1.0)
        deliveryPrice < fair -> ForwardPortfolio(-1.0, s0 / a0, 1.0)
        else -> null
    }
}

/**
 * Exercise 1.6: forward on a currency. Holding the foreign currency earns the foreign rate r_f
 * (a negative carrying cost), so F = S(0)(1+r)/(1+r_f) and r_f = S(0)(1+r)/F - 1.
 */
fun impliedForeignRate(s0: Double, deliveryPrice: Double, r: Double) =
    s0 * (1 + r) / deliveryPrice - 1
