package m4f.ch10

import kotlin.math.exp
import kotlin.math.ln

// Section 10.1: maturity-independent yields. All bonds are priced with one
// continuously compounded yield y, so B(t,T) = e^(-y(T-t)). A Bond is a list of
// cash flows (t_i, C_i); coupons and the face value are flows.

data class Flow(val t: Double, val C: Double)
typealias Bond = List<Flow>

fun zeroCoupon(face: Double, T: Double): Bond = listOf(Flow(T, face))

/** n coupons of `coupon` at dt, 2dt, ..., n dt; the face is repaid with the last. */
fun couponBond(face: Double, coupon: Double, n: Int, dt: Double = 1.0): Bond =
    List(n) { i -> Flow((i + 1) * dt, coupon + (if (i == n - 1) face else 0.0)) }

/** P(t) = sum over t_i > t of C_i e^(-y(t_i - t)) */
fun price(bond: Bond, y: Double, t: Double = 0.0): Double {
    var p = 0.0
    for (fl in bond) if (fl.t > t) p += fl.C * exp(-y * (fl.t - t))
    return p
}

/** D = sum (t_i - t) C_i e^(-y(t_i - t)) / P; dP/dy = -D P */
fun duration(bond: Bond, y: Double, t: Double = 0.0): Double {
    var s = 0.0
    for (fl in bond) if (fl.t > t) s += (fl.t - t) * fl.C * exp(-y * (fl.t - t))
    return s / price(bond, y, t)
}

/** 10.1.1: value at time t of one bond bought at 0, if the yield is y from then
 *  on: coupons received before t are reinvested at y, the rest is priced at y. */
fun investmentValue(bond: Bond, y: Double, t: Double): Double {
    var v = 0.0
    for (fl in bond) v += if (fl.t <= t) fl.C * exp(y * (t - fl.t)) else fl.C * exp(-y * (fl.t - t))
    return v
}

/** Annualised log return over [0,t] when buying at yield y0 and the yield jumps to y. */
fun logReturn(bond: Bond, y0: Double, y: Double, t: Double): Double = ln(investmentValue(bond, y, t) / price(bond, y0)) / t

/** 10.1.3 Portfolios of two bonds: the duration of a portfolio is the value-weighted
 *  average of durations. Weights w1 + w2 = 1 with w1 D1 + w2 D2 = D_target. */
fun immunisingWeights(D1: Double, D2: Double, Dtarget: Double): Pair<Double, Double> {
    val w1 = (Dtarget - D2) / (D1 - D2)
    return Pair(w1, 1 - w1)
}

/** q1, q2 = numbers of bond 1 and bond 2 */
data class Portfolio(val q1: Double, val q2: Double, val cash: Double)

fun value(p: Portfolio, b1: Bond, b2: Bond, y: Double, t: Double): Double =
    p.q1 * price(b1, y, t) + p.q2 * price(b2, y, t) + p.cash

/** Invest V at time t in bonds 1 and 2 so that the duration equals horizon - t. */
fun immunise(V: Double, b1: Bond, b2: Bond, y: Double, t: Double, horizon: Double): Portfolio {
    val (w1, w2) = immunisingWeights(duration(b1, y, t), duration(b2, y, t), horizon - t)
    return Portfolio(w1 * V / price(b1, y, t), w2 * V / price(b2, y, t), 0.0)
}

/** duration = of the rebalanced portfolio */
data class HedgeStep(val t: Double, val y: Double, val value: Double, val q1: Double, val q2: Double, val duration: Double)

/** 10.1.4: along a path of (t, y) with t[0] = 0, hold the two bonds, collect
 *  coupons into cash (accruing at the yield of the interval), and, if `rebalance`,
 *  re-immunise at each step so the duration equals the remaining horizon. */
fun dynamicHedge(V0: Double, b1: Bond, b2: Bond, horizon: Double, path: List<Pair<Double, Double>>, rebalance: Boolean = true): List<HedgeStep> {
    val (t0, y0) = path[0]
    var p = immunise(V0, b1, b2, y0, t0, horizon)
    val steps = arrayListOf(HedgeStep(t0, y0, V0, p.q1, p.q2, horizon - t0))
    for (i in 1 until path.size) {
        val (tPrev, yPrev) = path[i - 1]
        val (t, y) = path[i]
        var cash = p.cash * exp(yPrev * (t - tPrev))
        for ((q, b) in listOf(p.q1 to b1, p.q2 to b2))
            for (fl in b) if (tPrev < fl.t && fl.t <= t) cash += q * fl.C * exp(yPrev * (t - fl.t))
        p = Portfolio(p.q1, p.q2, cash)
        val V = value(p, b1, b2, y, t)
        if (rebalance && t < horizon) p = immunise(V, b1, b2, y, t, horizon)
        val D = if (t < horizon) (p.q1 * price(b1, y, t) * duration(b1, y, t) + p.q2 * price(b2, y, t) * duration(b2, y, t)) / V
        else 0.0
        steps.add(HedgeStep(t, y, V, p.q1, p.q2, D))
    }
    return steps
}
