// Mathematics for Finance (Capinski & Zastawniak), Chapter 1:
// "Introduction: A Simple Market Model" -- the one-step binomial market.
//
// Two assets, two time instants t = 0, 1:
//   risk-free bond  A(0) -> A(1)                     (return r)
//   risky stock     S(0) -> S^u(1) with prob. p      (return u)
//                         S^d(1) with prob. 1 - p    (return d)
// A portfolio (x, y) holds x shares and y bonds; negative = short / borrowed.
package m4f.ch01

import kotlin.math.abs
import kotlin.math.sqrt

enum class State { Up, Down }

data class Portfolio(val x: Double, val y: Double)

class OneStepMarket(
    val A0: Double, val A1: Double,
    val S0: Double, val Su: Double, val Sd: Double,
    val p: Double,
) {
    init {
        require(A0 > 0 && A1 > 0 && S0 > 0 && Sd > 0 && Su > Sd) { "require positive prices and S^u(1) > S^d(1)" }
        require(p > 0 && p < 1) { "require 0 < p < 1" }
    }

    fun S1(s: State) = if (s == State.Up) Su else Sd

    // Returns K = (P(1) - P(0)) / P(0)
    val r get() = (A1 - A0) / A0
    val u get() = (Su - S0) / S0
    val d get() = (Sd - S0) / S0

    // Theorem: no arbitrage  <=>  d < r < u
    val isArbitrageFree get() = d < r && r < u

    // Risk-neutral probability p* = (r - d) / (u - d); makes E*[S(1)] = (1+r) S(0)
    val riskNeutralProb get() = (r - d) / (u - d)

    // Portfolio investing `budget` with fraction `w` in stock, the rest in bonds.
    fun split(budget: Double, w: Double) = Portfolio(w * budget / S0, (1 - w) * budget / A0)

    fun value0(P: Portfolio) = P.x * S0 + P.y * A0
    fun value1(P: Portfolio, s: State) = P.x * S1(s) + P.y * A1
    fun portfolioReturn(P: Portfolio, s: State) = (value1(P, s) - value0(P)) / value0(P)

    // Expectation and standard deviation of a two-point random variable
    // taking values `up` (prob p) and `down` (prob 1-p).
    fun expect(up: Double, down: Double) = p * up + (1 - p) * down
    fun stddev(up: Double, down: Double) = sqrt(p * (1 - p)) * abs(up - down)

    // Replicating portfolio for a derivative paying Du (up) / Dd (down) at t=1:
    //   x S^u(1) + y A(1) = Du,   x S^d(1) + y A(1) = Dd
    fun replicate(Du: Double, Dd: Double): Portfolio {
        val x = (Du - Dd) / (Su - Sd)
        return Portfolio(x, (Du - x * Su) / A1)
    }

    // No-arbitrage price D(0) = value of the replicating portfolio ...
    fun price(Du: Double, Dd: Double) = value0(replicate(Du, Dd))

    // ... which equals the discounted risk-neutral expectation.
    fun riskNeutralPrice(Du: Double, Dd: Double): Double {
        val q = riskNeutralProb
        return (q * Du + (1 - q) * Dd) / (1 + r)
    }

    // Forward price for delivery at t=1: F = S(0) (1 + r)
    val forwardPrice get() = S0 * (1 + r)

    // If the model admits arbitrage, a portfolio with V(0)=0, V(1)>=0 and
    // V(1)>0 in some state, using one share; otherwise null.
    fun arbitrage(): Portfolio? = when {
        r >= u -> Portfolio(-1.0, S0 / A0)  // short stock, buy bonds
        r <= d -> Portfolio(1.0, -S0 / A0)  // borrow, buy stock
        else -> null
    }
}

fun callPayoff(S: Double, X: Double) = if (S > X) S - X else 0.0
fun putPayoff(S: Double, X: Double) = if (X > S) X - S else 0.0

// Section 1.7: cost at t=1 of buying one share for an investor who at t=0
// borrowed to buy n calls with strike X: S(1) - n C(1) + n C(0)(1+r).
fun hedgedPurchaseCost(m: OneStepMarket, X: Double, n: Double, s: State): Double {
    val C0 = m.price(callPayoff(m.Su, X), callPayoff(m.Sd, X))
    return m.S1(s) - n * callPayoff(m.S1(s), X) + n * C0 * (1 + m.r)
}
