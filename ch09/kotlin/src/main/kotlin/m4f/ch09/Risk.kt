package m4f.ch09

import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.sqrt

// Sections 9.2 and 9.3: Value at Risk and speculation under a lognormal stock,
//     S(T) = S(0) exp(m T + sigma sqrt(T) Z),  Z ~ N(0,1),
// so ln S(T)/S(0) is normal with mean m T and variance sigma^2 T. m is the
// expected logarithmic return; the Black-Scholes r only enters option prices.
//
// A Strategy is shares + cash + long/short options held to T. Its terminal
// value V(S(T)) is assumed non-decreasing in S(T) (long stock, long calls, long
// puts hedging stock, bull spreads), so quantiles of V are V at quantiles of S(T).

data class Lognormal(val S0: Double, val m: Double, val sigma: Double, val T: Double) {
    fun quantile(p: Double): Double = S0 * exp(m * T + sigma * sqrt(T) * normInv(p))

    /** P(S(T) <= s) */
    fun cdf(s: Double): Double = normCdf((ln(s / S0) - m * T) / (sigma * sqrt(T)))

    fun expectedStock(): Double = S0 * exp(m * T + sigma * sigma * T / 2)

    /** E max(S(T)-X, 0) = S0 e^((m+sigma^2/2)T) N(delta1) - X N(delta2) */
    fun expectedCallPayoff(X: Double): Double {
        val sq = sigma * sqrt(T)
        val delta2 = (ln(S0 / X) + m * T) / sq
        val delta1 = delta2 + sq
        return expectedStock() * normCdf(delta1) - X * normCdf(delta2)
    }

    /** E max(X-S(T), 0) = E max(S(T)-X, 0) - E S(T) + X */
    fun expectedPutPayoff(X: Double): Double = expectedCallPayoff(X) - expectedStock() + X
}

data class OptionLeg(val qty: Double, val X: Double, val isCall: Boolean)

/** cash is invested in the money market at r. */
data class Strategy(val name: String, val shares: Double, val cash: Double, val legs: List<OptionLeg>) {
    fun terminalValue(S: Double, r: Double, T: Double): Double {
        var v = shares * S + cash * exp(r * T)
        for (leg in legs) v += leg.qty * payoff(S, leg.X, leg.isCall)
        return v
    }

    fun expectedValue(model: Lognormal, r: Double): Double {
        var v = shares * model.expectedStock() + cash * exp(r * model.T)
        for (leg in legs) v += leg.qty * (if (leg.isCall) model.expectedCallPayoff(leg.X) else model.expectedPutPayoff(leg.X))
        return v
    }
}

fun initialCost(st: Strategy, S0: Double, r: Double, sigma: Double, T: Double): Double {
    var c = st.shares * S0 + st.cash
    for (leg in st.legs) c += leg.qty * bsPrice(S0, leg.X, r, sigma, T, leg.isCall)
    return c
}

/** 9.2.1: VaR at confidence level conf = initial value minus the (1-conf)
 *  quantile of the terminal value (positive = a loss). */
fun valueAtRisk(st: Strategy, model: Lognormal, r: Double, sigma: Double, conf: Double): Double {
    val V0 = initialCost(st, model.S0, r, sigma, model.T)
    return V0 - st.terminalValue(model.quantile(1 - conf), r, model.T)
}

fun expectedReturn(st: Strategy, model: Lognormal, r: Double, sigma: Double): Double {
    val V0 = initialCost(st, model.S0, r, sigma, model.T)
    return st.expectedValue(model, r) / V0 - 1
}

/** Stock price at T where the strategy neither gains nor loses (bisection). */
fun breakeven(st: Strategy, model: Lognormal, r: Double, sigma: Double): Double {
    val V0 = initialCost(st, model.S0, r, sigma, model.T)
    var lo = 0.0
    var hi = 100.0 * model.S0
    repeat(200) {
        val mid = (lo + hi) / 2
        if (st.terminalValue(mid, r, model.T) < V0) lo = mid else hi = mid
    }
    return (lo + hi) / 2
}

fun probLoss(st: Strategy, model: Lognormal, r: Double, sigma: Double): Double = model.cdf(breakeven(st, model, r, sigma))

/** 9.2: a holding of `shares` shares, hedged by buying `h` puts per share at strike X. */
fun putHedgedHolding(name: String, shares: Double, X: Double, h: Double = 1.0): Strategy =
    Strategy(name, shares, 0.0, listOf(OptionLeg(h * shares, X, false)))

fun unhedgedHolding(shares: Double): Strategy = Strategy("stock", shares, 0.0, emptyList())

/** 9.3.1: spend capital V0 on one instrument. */
fun allInStock(V0: Double, S0: Double): Strategy = Strategy("stock", V0 / S0, 0.0, emptyList())

fun allInCalls(name: String, V0: Double, S0: Double, X: Double, r: Double, sigma: Double, T: Double): Strategy {
    val n = V0 / bsPrice(S0, X, r, sigma, T, true)
    return Strategy(name, 0.0, 0.0, listOf(OptionLeg(n, X, true)))
}

fun bullSpread(name: String, V0: Double, S0: Double, X1: Double, X2: Double, r: Double, sigma: Double, T: Double): Strategy {
    val n = V0 / (bsPrice(S0, X1, r, sigma, T, true) - bsPrice(S0, X2, r, sigma, T, true))
    return Strategy(name, 0.0, 0.0, listOf(OptionLeg(n, X1, true), OptionLeg(-n, X2, true)))
}

fun mix(name: String, a: Strategy, wa: Double, b: Strategy, wb: Double): Strategy {
    val legs = a.legs.map { OptionLeg(it.qty * wa, it.X, it.isCall) } + b.legs.map { OptionLeg(it.qty * wb, it.X, it.isCall) }
    return Strategy(name, a.shares * wa + b.shares * wb, a.cash * wa + b.cash * wb, legs)
}
