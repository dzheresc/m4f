// Section 2.2, money market: zero-coupon bonds, coupon bonds and the money
// market account. Bond prices are per unit face value unless F is given.
package m4f.ch02

import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.pow

// 2.2.1 Zero-coupon bonds. Price at 0 of a unit bond maturing at T, for an implied
// rate r compounded m times a year (null: continuously).
fun zeroCouponPrice(r: Double, T: Double, m: Int? = null) =
    if (m == null) exp(-r * T) else (1 + r / m).pow(-T * m)

// Rate implied by a unit bond price B with maturity T (inverse of zeroCouponPrice).
fun impliedRate(B: Double, T: Double, m: Int? = null) =
    if (m == null) -ln(B) / T else m * (B.pow(-1 / (T * m)) - 1)

// 2.2.2 Coupon bonds: n coupons C at the end of each period, face value F paid
// with the last coupon; y is the yield per period.
fun couponBondPrice(F: Double, C: Double, n: Int, y: Double): Double {
    var price = F * (1 + y).pow(-n)
    for (k in 1..n) price += C * (1 + y).pow(-k)
    return price
}

// Per-period yield y with couponBondPrice(F, C, n, y) = price (bisection; the
// price is strictly decreasing in y).
fun yieldToMaturity(price: Double, F: Double, C: Double, n: Int,
                    lo0: Double = -0.99, hi0: Double = 10.0, tol: Double = 1e-12): Double {
    var lo = lo0
    var hi = hi0
    var it = 0
    while (it++ < 200 && hi - lo >= tol) {
        val mid = (lo + hi) / 2
        if (couponBondPrice(F, C, n, mid) > price) lo = mid else hi = mid
    }
    return (lo + hi) / 2
}

// 2.2.3 Money market account: A(t) = A(0) e^(r t) at a constant continuous rate.
fun moneyMarketValue(A0: Double, r: Double, t: Double) = A0 * exp(r * t)

// Money market account built by rolling over short zero-coupon bonds: at each
// step the whole balance buys bonds at unit price B, which pay 1 per bond.
fun moneyMarketRollover(A0: Double, prices: List<Double>): Double {
    var A = A0
    for (B in prices) A /= B
    return A
}
