// Sections 9.2 and 9.3: Value at Risk and speculation under a lognormal stock,
//     S(T) = S(0) exp(m T + sigma sqrt(T) Z),  Z ~ N(0,1),
// so ln S(T)/S(0) is normal with mean m T and variance sigma^2 T. m is the expected logarithmic
// return; the Black-Scholes r only enters option prices.
//
// A Strategy is shares + cash + long/short options held to T. Its terminal value V(S(T)) is
// assumed non-decreasing in S(T) (long stock, long calls, long puts hedging stock, bull spreads),
// so quantiles of V are V at quantiles of S(T).
package m4f.ch09

import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.sqrt

/** The lognormal model of S(T). */
data class Lognormal(val s0: Double, val m: Double, val sigma: Double, val maturity: Double) {
    /** The [p]-quantile of S(T). */
    fun quantile(p: Double): Double = s0 * exp(m * maturity + sigma * sqrt(maturity) * normInv(p))

    /** P(S(T) <= s). */
    fun cdf(s: Double): Double = normCdf((ln(s / s0) - m * maturity) / (sigma * sqrt(maturity)))

    /** E S(T). */
    fun expectedStock(): Double = s0 * exp(m * maturity + sigma * sigma * maturity / 2)

    /** E max(S(T)-X, 0) = S0 e^((m+sigma^2/2)T) N(delta1) - X N(delta2). */
    fun expectedCallPayoff(strike: Double): Double {
        val sq = sigma * sqrt(maturity)
        val delta2 = (ln(s0 / strike) + m * maturity) / sq
        val delta1 = delta2 + sq
        return expectedStock() * normCdf(delta1) - strike * normCdf(delta2)
    }

    /** E max(X-S(T), 0) = E max(S(T)-X, 0) - E S(T) + X. */
    fun expectedPutPayoff(strike: Double): Double =
        expectedCallPayoff(strike) - expectedStock() + strike
}

/** [qty] options (negative = written) with the given strike and type. */
data class OptionLeg(val qty: Double, val strike: Double, val isCall: Boolean)

/**
 * Shares, cash and options held to T.
 *
 * @property cash invested in the money market at r
 */
data class Strategy(
    val name: String,
    val shares: Double,
    val cash: Double,
    val legs: List<OptionLeg>,
) {
    /** Value at T when the stock ends at [s]. */
    fun terminalValue(s: Double, r: Double, maturity: Double): Double {
        var v = shares * s + cash * exp(r * maturity)
        for (leg in legs) {
            v += leg.qty * payoff(s, leg.strike, leg.isCall)
        }
        return v
    }

    /** E V(T) under the lognormal [model]. */
    fun expectedValue(model: Lognormal, r: Double): Double {
        var v = shares * model.expectedStock() + cash * exp(r * model.maturity)
        for (leg in legs) {
            val expected = if (leg.isCall) {
                model.expectedCallPayoff(leg.strike)
            } else {
                model.expectedPutPayoff(leg.strike)
            }
            v += leg.qty * expected
        }
        return v
    }
}

/** Cost of setting up the strategy at time 0. */
fun initialCost(st: Strategy, s0: Double, r: Double, sigma: Double, maturity: Double): Double {
    var c = st.shares * s0 + st.cash
    for (leg in st.legs) {
        c += leg.qty * bsPrice(s0, leg.strike, r, sigma, maturity, leg.isCall)
    }
    return c
}

/**
 * Section 9.2.1: VaR at confidence level [conf] = initial value minus the (1-conf) quantile of the
 * terminal value (positive = a loss).
 */
fun valueAtRisk(st: Strategy, model: Lognormal, r: Double, sigma: Double, conf: Double): Double {
    val v0 = initialCost(st, model.s0, r, sigma, model.maturity)
    return v0 - st.terminalValue(model.quantile(1 - conf), r, model.maturity)
}

/** E V(T) / V(0) - 1. */
fun expectedReturn(st: Strategy, model: Lognormal, r: Double, sigma: Double): Double {
    val v0 = initialCost(st, model.s0, r, sigma, model.maturity)
    return st.expectedValue(model, r) / v0 - 1
}

/** Stock price at T where the strategy neither gains nor loses (bisection). */
fun breakeven(st: Strategy, model: Lognormal, r: Double, sigma: Double): Double {
    val v0 = initialCost(st, model.s0, r, sigma, model.maturity)
    var lo = 0.0
    var hi = 100.0 * model.s0
    repeat(200) {
        val mid = (lo + hi) / 2
        if (st.terminalValue(mid, r, model.maturity) < v0) lo = mid else hi = mid
    }
    return (lo + hi) / 2
}

/** P(V(T) < V(0)). */
fun probLoss(st: Strategy, model: Lognormal, r: Double, sigma: Double): Double =
    model.cdf(breakeven(st, model, r, sigma))

/** Section 9.2: a holding of [shares] shares, hedged by buying [h] puts per share at [strike]. */
fun putHedgedHolding(name: String, shares: Double, strike: Double, h: Double = 1.0): Strategy =
    Strategy(name, shares, 0.0, listOf(OptionLeg(h * shares, strike, false)))

/** A plain holding of [shares] shares. */
fun unhedgedHolding(shares: Double): Strategy = Strategy("stock", shares, 0.0, emptyList())

/** Section 9.3.1: spend capital [v0] on the stock at price [s0] = S(0). */
fun allInStock(v0: Double, s0: Double): Strategy = Strategy("stock", v0 / s0, 0.0, emptyList())

/** Spend capital [v0] on calls. */
fun allInCalls(
    name: String,
    v0: Double,
    s0: Double,
    strike: Double,
    r: Double,
    sigma: Double,
    maturity: Double,
): Strategy {
    val n = v0 / bsPrice(s0, strike, r, sigma, maturity, true)
    return Strategy(name, 0.0, 0.0, listOf(OptionLeg(n, strike, true)))
}

/** Spend capital [v0] on a bull spread: long calls at [x1], short calls at [x2]. */
fun bullSpread(
    name: String,
    v0: Double,
    s0: Double,
    x1: Double,
    x2: Double,
    r: Double,
    sigma: Double,
    maturity: Double,
): Strategy {
    val n = v0 /
        (bsPrice(s0, x1, r, sigma, maturity, true) - bsPrice(s0, x2, r, sigma, maturity, true))
    return Strategy(name, 0.0, 0.0, listOf(OptionLeg(n, x1, true), OptionLeg(-n, x2, true)))
}

/** [wa] of strategy [a] plus [wb] of strategy [b]. */
fun mix(name: String, a: Strategy, wa: Double, b: Strategy, wb: Double): Strategy {
    val legs = a.legs.map { OptionLeg(it.qty * wa, it.strike, it.isCall) } +
        b.legs.map { OptionLeg(it.qty * wb, it.strike, it.isCall) }
    return Strategy(name, a.shares * wa + b.shares * wb, a.cash * wa + b.cash * wb, legs)
}
