// Section 1.5: forward contracts. A portfolio (x, y, z) adds z forward
// contracts (z > 0 long, z < 0 short) with forward price F to x shares and
// y bonds; entering a forward costs nothing at t=0.
package m4f.ch01

data class ForwardPortfolio(val x: Double, val y: Double, val z: Double) {
    fun value0(S0: Double, A0: Double) = x * S0 + y * A0
    fun value1(S1: Double, A1: Double, F: Double) = x * S1 + y * A1 + z * (S1 - F)
}

fun longForwardPayoff(S1: Double, F: Double) = S1 - F
fun shortForwardPayoff(S1: Double, F: Double) = F - S1

// Proposition 1.2: the No-Arbitrage forward price F = S(0) A(1) / A(0) = S(0)(1+r)
fun forwardPrice(S0: Double, A0: Double, A1: Double) = S0 * A1 / A0

// Cash-and-carry arbitrage against a mispriced forward, one contract:
//   F > fair: borrow S(0), buy a share, short the forward  -> V(1) = F - fair
//   F < fair: short a share, invest S(0), long the forward -> V(1) = fair - F
// V(0) = 0 and V(1) does not depend on S(1).
fun forwardArbitrage(S0: Double, A0: Double, A1: Double, F: Double): ForwardPortfolio? {
    val fair = forwardPrice(S0, A0, A1)
    return when {
        F > fair -> ForwardPortfolio(1.0, -S0 / A0, -1.0)
        F < fair -> ForwardPortfolio(-1.0, S0 / A0, 1.0)
        else -> null
    }
}

// Exercise 1.6: forward on a currency. Holding the foreign currency earns the
// foreign rate r_f (a negative carrying cost), so F = S(0)(1+r)/(1+r_f) and
//   r_f = S(0)(1+r)/F - 1.
fun impliedForeignRate(S0: Double, F: Double, r: Double) = S0 * (1 + r) / F - 1
