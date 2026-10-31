// Section 2.2, money market: zero-coupon bonds, coupon bonds and the money market account.
// Bond prices are per unit face value unless a face value is given.
//
// Book notation: T is the maturity (maturity), B a unit bond price (bondPrice), F the face
// value (face), C the coupon (coupon), y the yield per period, A(0) the initial balance (a0).
package m4f.ch02

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.expm1
import kotlin.math.ln
import kotlin.math.ln1p
import kotlin.math.pow

/**
 * 2.2.1 Zero-coupon bonds. Price at 0 of a unit bond maturing at [maturity], for an implied
 * rate [r] compounded [m] times a year (null: continuously).
 */
fun zeroCouponPrice(r: Double, maturity: Double, m: Int? = null) =
    if (m == null) exp(-r * maturity) else (1 + r / m).pow(-maturity * m)

/** Rate implied by a unit [bondPrice] with [maturity] (inverse of [zeroCouponPrice]). */
fun impliedRate(bondPrice: Double, maturity: Double, m: Int? = null) =
    if (m == null) -ln(bondPrice) / maturity else m * (bondPrice.pow(-1 / (maturity * m)) - 1)

/**
 * 2.2.2 Coupon bonds: [n] coupons [coupon] at the end of each period, [face] value paid with
 * the last coupon; [y] is the yield per period.
 *
 * The coupons form a geometric series, so price = F v + C (1 - v) / y with v = (1+y)^-n, and
 * 1 - v is computed as -expm1(-n log1p(y)) to stay accurate for small y.
 */
fun couponBondPrice(face: Double, coupon: Double, n: Int, y: Double): Double {
    if (y == 0.0) return face + coupon * n
    val logV = -n * ln1p(y)
    return face * exp(logV) - coupon * expm1(logV) / y
}

/** Derivative of couponBondPrice in [y]: -(n F v + C sum_k k (1+y)^-k) / (1+y). */
fun couponBondPriceSlope(face: Double, coupon: Double, n: Int, y: Double): Double {
    val discount = 1 / (1 + y)
    var factor = 1.0
    var weighted = 0.0
    for (k in 1..n) {
        factor *= discount
        weighted += k * factor
    }
    return -(n * face * factor + coupon * weighted) * discount
}

/**
 * Per-period yield y in [[lo0], [hi0]] with couponBondPrice(face, coupon, n, y) = [price].
 * Newton's method, kept inside a shrinking bisection bracket (the price is strictly decreasing
 * in y), so it falls back to bisection whenever a Newton step would leave the bracket and
 * returns the nearer end when the price is out of range. Stops once the bracket or the Newton
 * step is narrower than [tol]. [n] is the number of coupon periods.
 */
fun yieldToMaturity(
    price: Double,
    face: Double,
    coupon: Double,
    n: Int,
    lo0: Double = -0.99,
    hi0: Double = 10.0,
    tol: Double = 1e-12,
): Double {
    var lo = lo0
    var hi = hi0
    // Start from the textbook approximation (C + (F - P) / n) / P.
    var y = (coupon + (face - price) / n) / price
    if (!(y > lo && y < hi)) y = (lo + hi) / 2
    var iteration = 0
    while (iteration < 200 && hi - lo >= tol) {
        val excess = couponBondPrice(face, coupon, n, y) - price
        if (excess == 0.0) return y
        if (excess > 0) {
            lo = y
        } else {
            hi = y
        }
        var next = y - excess / couponBondPriceSlope(face, coupon, n, y)
        if (!(next > lo && next < hi)) next = (lo + hi) / 2
        if (abs(next - y) < tol) return next
        y = next
        iteration++
    }
    return (lo + hi) / 2
}

/** 2.2.3 Money market account: A(t) = A(0) e^(r t) at a constant continuous rate. */
fun moneyMarketValue(a0: Double, r: Double, t: Double) = a0 * exp(r * t)

/**
 * Money market account built by rolling over short zero-coupon bonds: at each step the whole
 * balance buys bonds at unit price B, which pay 1 per bond.
 */
fun moneyMarketRollover(a0: Double, bondPrices: List<Double>): Double {
    var balance = a0
    for (bondPrice in bondPrices) {
        balance /= bondPrice
    }
    return balance
}
