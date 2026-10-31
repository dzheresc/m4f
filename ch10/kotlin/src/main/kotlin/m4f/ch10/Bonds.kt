package m4f.ch10

import kotlin.math.exp
import kotlin.math.ln

// Section 10.1: maturity-independent yields. All bonds are priced with one continuously compounded
// yield y, so B(t,T) = e^(-y(T-t)). A Bond is a list of cash flows (t_i, C_i); coupons and the
// face value are flows.

/**
 * One cash flow of a bond.
 *
 * @property t payment time in years.
 * @property amount amount paid at [t].
 */
data class Flow(
    val t: Double,
    val amount: Double,
)

typealias Bond = List<Flow>

/** A zero-coupon bond paying [face] at [maturity]. */
fun zeroCoupon(face: Double, maturity: Double): Bond = listOf(Flow(maturity, face))

/** [n] coupons of [coupon] at dt, 2dt, ..., n dt; the face is repaid with the last. */
fun couponBond(face: Double, coupon: Double, n: Int, dt: Double = 1.0): Bond =
    List(n) { i -> Flow((i + 1) * dt, coupon + (if (i == n - 1) face else 0.0)) }

/** P(t) = sum over t_i > t of C_i e^(-y(t_i - t)). */
fun price(bond: Bond, y: Double, t: Double = 0.0): Double {
    var p = 0.0
    for (flow in bond) {
        if (flow.t > t) p += flow.amount * exp(-y * (flow.t - t))
    }
    return p
}

/** Price and duration of a bond at yield y and time t, from one pass over the cash flows. */
data class PriceDuration(val price: Double, val duration: Double)

/** [price] and [duration] of [bond] together, from one pass over the cash flows. */
fun priceAndDuration(bond: Bond, y: Double, t: Double = 0.0): PriceDuration {
    var p = 0.0
    var s = 0.0
    for (flow in bond) {
        if (flow.t > t) {
            val e = exp(-y * (flow.t - t))
            p += flow.amount * e
            s += (flow.t - t) * flow.amount * e
        }
    }
    return PriceDuration(p, s / p)
}

/** D = sum (t_i - t) C_i e^(-y(t_i - t)) / P; dP/dy = -D P. */
fun duration(bond: Bond, y: Double, t: Double = 0.0): Double = priceAndDuration(bond, y, t).duration

/**
 * 10.1.1: value at time [t] of one bond bought at 0, if the yield is [y] from then on: coupons
 * received before [t] are reinvested at [y], the rest is priced at [y].
 */
fun investmentValue(bond: Bond, y: Double, t: Double): Double {
    var v = 0.0
    for (flow in bond) {
        v +=
            if (flow.t <= t) {
                flow.amount * exp(y * (t - flow.t))
            } else {
                flow.amount * exp(-y * (flow.t - t))
            }
    }
    return v
}

/** Annualised log return over [0, t] when buying at yield [y0] and the yield jumps to [y]. */
fun logReturn(bond: Bond, y0: Double, y: Double, t: Double): Double =
    ln(investmentValue(bond, y, t) / price(bond, y0)) / t

/**
 * 10.1.3 Portfolios of two bonds: the duration of a portfolio is the value-weighted average of
 * durations. Returns weights w1 + w2 = 1 with w1 D1 + w2 D2 = [targetDuration], where D1 and D2
 * are the durations of the two bonds.
 */
fun immunisingWeights(
    duration1: Double,
    duration2: Double,
    targetDuration: Double,
): Pair<Double, Double> {
    val w1 = (targetDuration - duration2) / (duration1 - duration2)
    return Pair(w1, 1 - w1)
}

/**
 * A holding of two bonds plus cash.
 *
 * @property q1 number of bond 1.
 * @property q2 number of bond 2.
 * @property cash cash held.
 */
data class Portfolio(
    val q1: Double,
    val q2: Double,
    val cash: Double,
)

/** Value of [p] at time [t] when both bonds are priced at yield [y]. */
fun portfolioValue(p: Portfolio, b1: Bond, b2: Bond, y: Double, t: Double): Double =
    p.q1 * price(b1, y, t) + p.q2 * price(b2, y, t) + p.cash

/**
 * Invest [value] in bonds 1 and 2, given their prices and durations [b1] and [b2], so that the
 * duration equals [targetDuration].
 */
fun immunise(
    value: Double,
    b1: PriceDuration,
    b2: PriceDuration,
    targetDuration: Double,
): Portfolio {
    val (w1, w2) = immunisingWeights(b1.duration, b2.duration, targetDuration)
    return Portfolio(w1 * value / b1.price, w2 * value / b2.price, 0.0)
}

/** Invest [value] at time [t] in bonds 1 and 2 so that the duration equals horizon - t. */
fun immunise(value: Double, b1: Bond, b2: Bond, y: Double, t: Double, horizon: Double): Portfolio =
    immunise(value, priceAndDuration(b1, y, t), priceAndDuration(b2, y, t), horizon - t)

/**
 * One step of a dynamic hedge.
 *
 * @property t time.
 * @property y yield at [t].
 * @property value portfolio value at [t].
 * @property q1 number of bond 1 after rebalancing.
 * @property q2 number of bond 2 after rebalancing.
 * @property duration duration of the rebalanced portfolio.
 */
data class HedgeStep(
    val t: Double,
    val y: Double,
    val value: Double,
    val q1: Double,
    val q2: Double,
    val duration: Double,
)

/**
 * 10.1.4: along a [path] of (t, y) with t[0] = 0, hold the two bonds, collect coupons into cash
 * (accruing at the yield of the interval), and, if [rebalance], re-immunise at each step so the
 * duration equals the remaining horizon. [v0] is the capital invested at t = 0.
 */
fun dynamicHedge(
    value0: Double,
    b1: Bond,
    b2: Bond,
    horizon: Double,
    path: List<Pair<Double, Double>>,
    rebalance: Boolean = true,
): List<HedgeStep> {
    val (t0, y0) = path[0]
    var p = immunise(value0, b1, b2, y0, t0, horizon)
    val steps = arrayListOf(HedgeStep(t0, y0, value0, p.q1, p.q2, horizon - t0))
    for (i in 1 until path.size) {
        val (tPrev, yPrev) = path[i - 1]
        val (t, y) = path[i]
        var cash = p.cash * exp(yPrev * (t - tPrev))
        for ((q, b) in listOf(p.q1 to b1, p.q2 to b2)) {
            for (flow in b) {
                if (tPrev < flow.t && flow.t <= t) {
                    cash += q * flow.amount * exp(yPrev * (t - flow.t))
                }
            }
        }
        // Both bonds are priced once per step; the values equal price() and duration() bit for
        // bit.
        val pd1 = priceAndDuration(b1, y, t)
        val pd2 = priceAndDuration(b2, y, t)
        p = Portfolio(p.q1, p.q2, cash)
        val value = p.q1 * pd1.price + p.q2 * pd2.price + p.cash
        if (rebalance && t < horizon) p = immunise(value, pd1, pd2, horizon - t)
        val d =
            if (t < horizon) {
                (p.q1 * pd1.price * pd1.duration + p.q2 * pd2.price * pd2.duration) / value
            } else {
                0.0
            }
        steps.add(HedgeStep(t, y, value, p.q1, p.q2, d))
    }
    return steps
}
