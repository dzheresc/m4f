// Section 9.1: hedging option positions with the stock, the money market and other options. A
// Position is a number of options (negative = written) with a given strike, expiry and type; all
// options are on the same stock, priced by Black-Scholes with a common r and sigma.
package m4f.ch09

import kotlin.math.exp

/** [qty] options (negative = written) with the given strike, expiry and type. */
data class Position(val qty: Double, val strike: Double, val expiry: Double, val isCall: Boolean)

/** Options held, plus shares and money market balance at time 0. */
data class Hedge(val options: List<Position>, val shares: Double, val money: Double)

/** Value at time [t] when the stock is at [s] (money accrues at e^(rt)). */
fun portfolioValue(h: Hedge, s: Double, t: Double, r: Double, sigma: Double): Double {
    var v = h.shares * s + h.money * exp(r * t)
    for (p in h.options) {
        v += p.qty * bsPrice(s, p.strike, r, sigma, p.expiry - t, p.isCall)
    }
    return v
}

/** Sum of qty times the Greek [which] over the options. */
fun totalGreek(options: List<Position>, s: Double, r: Double, sigma: Double, which: Greek): Double {
    var g = 0.0
    for (p in options) {
        g += p.qty * bsGreeks(s, p.strike, r, sigma, p.expiry, p.isCall).get(which)
    }
    return g
}

private fun premium(options: List<Position>, s: Double, r: Double, sigma: Double): Double {
    var v = 0.0
    for (p in options) {
        v += p.qty * bsPrice(s, p.strike, r, sigma, p.expiry, p.isCall)
    }
    return -v
}

/**
 * Section 9.1.1: make the position delta neutral with shares; the money market absorbs the
 * premium received and the cost of the shares (value 0 at t=0).
 */
fun deltaHedge(options: List<Position>, s: Double, r: Double, sigma: Double): Hedge {
    val shares = -totalGreek(options, s, r, sigma, Greek.DELTA)
    return Hedge(options, shares, premium(options, s, r, sigma) - shares * s)
}

/**
 * Section 9.1.2: neutralise delta and one more Greek (gamma or vega) with shares and a position
 * in a second option [extra] (its qty is ignored and solved for).
 */
fun deltaGreekHedge(
    options: List<Position>,
    extra: Position,
    s: Double,
    r: Double,
    sigma: Double,
    which: Greek,
): Hedge {
    val gExtra = bsGreeks(s, extra.strike, r, sigma, extra.expiry, extra.isCall).get(which)
    val qty = -totalGreek(options, s, r, sigma, which) / gExtra
    val all = options + Position(qty, extra.strike, extra.expiry, extra.isCall)
    val shares = -totalGreek(all, s, r, sigma, Greek.DELTA)
    return Hedge(all, shares, premium(all, s, r, sigma) - shares * s)
}

/**
 * One rebalancing date.
 *
 * @property value mark-to-market value of the whole hedged position
 */
data class RebalanceStep(
    val t: Double,
    val s: Double,
    val shares: Double,
    val money: Double,
    val value: Double,
)

/**
 * Dynamic delta hedging: re-hedge at each price on the path ([prices][0] at t=0, then one price
 * every [dt] years).
 */
fun rebalancePath(
    options: List<Position>,
    prices: List<Double>,
    dt: Double,
    r: Double,
    sigma: Double,
): List<RebalanceStep> {
    val steps = ArrayList<RebalanceStep>()
    val h = deltaHedge(options, prices[0], r, sigma)
    var shares = h.shares
    var money = h.money
    steps.add(
        RebalanceStep(0.0, prices[0], shares, money, portfolioValue(h, prices[0], 0.0, r, sigma)),
    )
    for (i in 1 until prices.size) {
        val t = i * dt
        val s = prices[i]
        money = money * exp(r * dt)
        val shifted = options.map { Position(it.qty, it.strike, it.expiry - t, it.isCall) }
        val newShares = -totalGreek(shifted, s, r, sigma, Greek.DELTA)
        money -= (newShares - shares) * s
        shares = newShares
        var value = shares * s + money
        for (p in options) {
            value += p.qty * bsPrice(s, p.strike, r, sigma, p.expiry - t, p.isCall)
        }
        steps.add(RebalanceStep(t, s, shares, money, value))
    }
    return steps
}
