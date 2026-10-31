// Mathematics for Finance (Capinski & Zastawniak), Chapter 1:
// "Introduction: A Simple Market Model" -- the one-step binomial market.
//
// Two assets, two time instants t = 0, 1:
//   risk-free bond  A(0) -> A(1)                     (return r)
//   risky stock     S(0) -> S^u(1) with prob. p      (return u)
//                         S^d(1) with prob. 1 - p    (return d)
// A portfolio (x, y) holds x shares and y bonds; negative = short / borrowed.
//
// Names follow the book's notation in lowerCamelCase:
// a0 = A(0), a1 = A(1), s0 = S(0), sUp = S^u(1), sDown = S^d(1).
package m4f.ch01

import kotlin.math.abs
import kotlin.math.sqrt

enum class State { UP, DOWN }

/** A holding of [x] shares of stock and [y] units of bond. */
data class Portfolio(val x: Double, val y: Double)

/**
 * Bond A(0) -> A(1) and stock S(0) -> S^u(1) | S^d(1) with probability [p].
 *
 * @throws IllegalArgumentException unless all prices are positive, sUp > sDown and 0 < p < 1.
 */
class OneStepMarket(
    val a0: Double,
    val a1: Double,
    val s0: Double,
    val sUp: Double,
    val sDown: Double,
    val p: Double,
) {
    init {
        require(a0 > 0 && a1 > 0 && s0 > 0 && sDown > 0 && sUp > sDown) {
            "require positive prices and S^u(1) > S^d(1)"
        }
        require(p > 0 && p < 1) { "require 0 < p < 1" }
    }

    fun s1(s: State) = if (s == State.UP) sUp else sDown

    /** Bond return K_A = (A(1) - A(0)) / A(0). */
    val r get() = (a1 - a0) / a0

    /** Stock return in the up state. */
    val u get() = (sUp - s0) / s0

    /** Stock return in the down state. */
    val d get() = (sDown - s0) / s0

    /** Theorem: no arbitrage <=> d < r < u. */
    val isArbitrageFree get() = d < r && r < u

    /** Risk-neutral probability p* = (r - d) / (u - d), so that E*[S(1)] = (1 + r) S(0). */
    val riskNeutralProb get() = (r - d) / (u - d)

    /** Portfolio investing [budget] with fraction [w] in stock, the rest in bonds. */
    fun split(budget: Double, w: Double) = Portfolio(w * budget / s0, (1 - w) * budget / a0)

    fun value0(portfolio: Portfolio) = portfolio.x * s0 + portfolio.y * a0

    fun value1(portfolio: Portfolio, s: State) = portfolio.x * s1(s) + portfolio.y * a1

    fun portfolioReturn(portfolio: Portfolio, s: State) =
        (value1(portfolio, s) - value0(portfolio)) / value0(portfolio)

    /** Expectation of a variable equal to [up] (probability p) or [down]. */
    fun expect(up: Double, down: Double) = p * up + (1 - p) * down

    /** Standard deviation of a variable equal to [up] (probability p) or [down]. */
    fun stddev(up: Double, down: Double) = sqrt(p * (1 - p)) * abs(up - down)

    /**
     * Replicating portfolio for a derivative paying [dUp] / [dDown] at t=1:
     * x S^u(1) + y A(1) = D^u, x S^d(1) + y A(1) = D^d.
     */
    fun replicate(dUp: Double, dDown: Double): Portfolio {
        val x = (dUp - dDown) / (sUp - sDown)
        return Portfolio(x, (dUp - x * sUp) / a1)
    }

    /** No-arbitrage price D(0) = value of the replicating portfolio ... */
    fun price(dUp: Double, dDown: Double) = value0(replicate(dUp, dDown))

    /** ... which equals the discounted risk-neutral expectation. */
    fun riskNeutralPrice(dUp: Double, dDown: Double): Double {
        val q = riskNeutralProb
        return (q * dUp + (1 - q) * dDown) / (1 + r)
    }

    /** Forward price for delivery at t=1: F = S(0) (1 + r). */
    val forwardPrice get() = s0 * (1 + r)

    /**
     * If the model admits arbitrage, a portfolio with V(0) = 0, V(1) >= 0 and V(1) > 0 in some
     * state, using one share; otherwise null.
     */
    fun arbitrage(): Portfolio? = when {
        r >= u -> Portfolio(-1.0, s0 / a0) // Short one share, buy bonds with the proceeds.
        r <= d -> Portfolio(1.0, -s0 / a0) // Borrow to buy one share.
        else -> null
    }
}

fun callPayoff(s: Double, strike: Double) = if (s > strike) s - strike else 0.0

fun putPayoff(s: Double, strike: Double) = if (strike > s) strike - s else 0.0

/**
 * Section 1.7: cost at t=1 of buying one share for an investor who at t=0 borrowed to buy
 * [numCalls] calls with strike [strike]: S(1) - n C(1) + n C(0) (1 + r).
 */
fun hedgedPurchaseCost(market: OneStepMarket, strike: Double, numCalls: Double, s: State): Double {
    val call0 = market.price(callPayoff(market.sUp, strike), callPayoff(market.sDown, strike))
    return market.s1(s) - numCalls * callPayoff(market.s1(s), strike) +
        numCalls * call0 * (1 + market.r)
}
