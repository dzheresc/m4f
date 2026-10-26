package m4f.ch09

import kotlin.math.exp

// Section 9.1: hedging option positions with the stock, the money market and
// other options. A Position is a number of options (negative = written) with a
// given strike, expiry and type; all options are on the same stock, priced by
// Black-Scholes with a common r and sigma.

data class Position(val qty: Double, val X: Double, val T: Double, val isCall: Boolean)

/** Options held, plus shares and money market balance at time 0. */
data class Hedge(val options: List<Position>, val shares: Double, val money: Double)

/** Value at time t when the stock is at S (money accrues at e^(rt)). */
fun portfolioValue(h: Hedge, S: Double, t: Double, r: Double, sigma: Double): Double {
    var v = h.shares * S + h.money * exp(r * t)
    for (p in h.options) v += p.qty * bsPrice(S, p.X, r, sigma, p.T - t, p.isCall)
    return v
}

fun totalGreek(options: List<Position>, S: Double, r: Double, sigma: Double, which: Greek): Double {
    var g = 0.0
    for (p in options) g += p.qty * bsGreeks(S, p.X, r, sigma, p.T, p.isCall).get(which)
    return g
}

private fun premium(options: List<Position>, S: Double, r: Double, sigma: Double): Double {
    var v = 0.0
    for (p in options) v += p.qty * bsPrice(S, p.X, r, sigma, p.T, p.isCall)
    return -v
}

/** 9.1.1: make the position delta neutral with shares; the money market
 *  absorbs the premium received and the cost of the shares (value 0 at t=0). */
fun deltaHedge(options: List<Position>, S: Double, r: Double, sigma: Double): Hedge {
    val shares = -totalGreek(options, S, r, sigma, Greek.DELTA)
    return Hedge(options, shares, premium(options, S, r, sigma) - shares * S)
}

/** 9.1.2: neutralise delta and one more Greek (gamma or vega) with shares and a
 *  position in a second option `extra` (its qty is ignored and solved for). */
fun deltaGreekHedge(options: List<Position>, extra: Position, S: Double, r: Double, sigma: Double, which: Greek): Hedge {
    val gExtra = bsGreeks(S, extra.X, r, sigma, extra.T, extra.isCall).get(which)
    val qty = -totalGreek(options, S, r, sigma, which) / gExtra
    val all = options + Position(qty, extra.X, extra.T, extra.isCall)
    val shares = -totalGreek(all, S, r, sigma, Greek.DELTA)
    return Hedge(all, shares, premium(all, S, r, sigma) - shares * S)
}

/** value = mark-to-market value of the whole hedged position */
data class RebalanceStep(val t: Double, val S: Double, val shares: Double, val money: Double, val value: Double)

/** Dynamic delta hedging: re-hedge at each price on the path (prices[0] at t=0). */
fun rebalancePath(options: List<Position>, prices: List<Double>, dt: Double, r: Double, sigma: Double): List<RebalanceStep> {
    val steps = ArrayList<RebalanceStep>()
    val h = deltaHedge(options, prices[0], r, sigma)
    var shares = h.shares
    var money = h.money
    steps.add(RebalanceStep(0.0, prices[0], shares, money, portfolioValue(h, prices[0], 0.0, r, sigma)))
    for (i in 1 until prices.size) {
        val t = i * dt
        val S = prices[i]
        money = money * exp(r * dt)
        val shifted = options.map { Position(it.qty, it.X, it.T - t, it.isCall) }
        val newShares = -totalGreek(shifted, S, r, sigma, Greek.DELTA)
        money -= (newShares - shares) * S
        shares = newShares
        var value = shares * S + money
        for (p in options) value += p.qty * bsPrice(S, p.X, r, sigma, p.T - t, p.isCall)
        steps.add(RebalanceStep(t, S, shares, money, value))
    }
    return steps
}
