// Section 2.2, money market: zero-coupon bonds, coupon bonds and the money market account.
// Bond prices are per unit face value unless a face value is given.
//
// Book notation: T is the maturity (maturity), B a unit bond price (bondPrice), F the face
// value (face), C the coupon (coupon), y the yield per period, A(0) the initial balance (a0).
package m4f.ch02

import kotlin.math.exp
import kotlin.math.ln
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
 */
fun couponBondPrice(face: Double, coupon: Double, n: Int, y: Double): Double {
    var price = face * (1 + y).pow(-n)
    for (k in 1..n) {
        price += coupon * (1 + y).pow(-k)
    }
    return price
}

/**
 * Per-period yield y with couponBondPrice(face, coupon, n, y) = [price], found by bisection on
 * [[lo0], [hi0]] (the price is strictly decreasing in y).
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
    var iteration = 0
    while (iteration < 200 && hi - lo >= tol) {
        val mid = (lo + hi) / 2
        if (couponBondPrice(face, coupon, n, mid) > price) {
            lo = mid
        } else {
            hi = mid
        }
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
