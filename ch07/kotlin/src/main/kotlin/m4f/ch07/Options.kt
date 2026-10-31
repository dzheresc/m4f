// Sections 7.1-7.3: option payoffs, put-call parity and bounds on option prices.
// Continuous compounding at rate r; `maturity` is the time T to expiry in years and `strike` the
// strike X. `div` is the present value of dividends paid before T. `s0` is the stock price S(0)
// now and `s` the stock price S(T) at expiry.
package m4f.ch07

import kotlin.math.exp
import kotlin.math.max

// Section 7.1: definitions.

/** max(S - X, 0). */
fun callPayoff(s: Double, strike: Double): Double = max(s - strike, 0.0)

/** max(X - S, 0). */
fun putPayoff(s: Double, strike: Double): Double = max(strike - s, 0.0)

/**
 * Profit at T of a long position bought for [premium] at 0 (premium financed at r); a short
 * position earns the negative of it.
 */
fun profit(payoff: Double, premium: Double, r: Double, maturity: Double): Double =
    payoff - premium * exp(r * maturity)

/**
 * Section 7.2: put-call parity, C - P = S(0) - div - X e^(-rT).
 * Returns C - P - (S(0) - div - X e^(-rT)). Positive: write the call, buy the put and the stock,
 * borrow; negative: the reverse. Either locks in |gap| e^(rT) at T.
 */
fun parityGap(
    call: Double,
    put: Double,
    s0: Double,
    strike: Double,
    r: Double,
    maturity: Double,
    div: Double = 0.0,
): Double = call - put - (s0 - div - strike * exp(-r * maturity))

/** A price interval [lo, hi]. */
data class Bounds(val lo: Double, val hi: Double)

// Section 7.3.1: European options.

/** max(0, S(0) - div - X e^(-rT)) <= C_E <= S(0) - div. */
fun europeanCallBounds(
    s0: Double,
    strike: Double,
    r: Double,
    maturity: Double,
    div: Double = 0.0,
): Bounds = Bounds(max(0.0, s0 - div - strike * exp(-r * maturity)), s0 - div)

/** max(0, X e^(-rT) - S(0) + div) <= P_E <= X e^(-rT). */
fun europeanPutBounds(
    s0: Double,
    strike: Double,
    r: Double,
    maturity: Double,
    div: Double = 0.0,
): Bounds =
    Bounds(max(0.0, strike * exp(-r * maturity) - s0 + div), strike * exp(-r * maturity))

/**
 * How far a quoted price lies outside [lo, hi] (0 if inside); this much can be locked in at time
 * 0 by trading against the quote.
 */
fun boundViolation(price: Double, b: Bounds): Double = when {
    price < b.lo -> b.lo - price
    price > b.hi -> price - b.hi
    else -> 0.0
}

/** Section 7.3.3: American options, S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) (no dividends). */
fun americanParityBounds(s0: Double, strike: Double, r: Double, maturity: Double): Bounds =
    Bounds(s0 - strike, s0 - strike * exp(-r * maturity))

/**
 * Section 7.4.1: dependence on the strike, for X1 < X2 < X3. Convexity in X:
 * C(X2) <= l C(X1) + (1-l) C(X3), l = (X3-X2)/(X3-X1). A positive return value is the arbitrage
 * profit from writing the middle option and buying the wings. [c1], [c2], [c3] are the call prices
 * at strikes [x1], [x2], [x3].
 */
fun butterflyGap(c1: Double, c2: Double, c3: Double, x1: Double, x2: Double, x3: Double): Double {
    val lam = (x3 - x2) / (x3 - x1)
    return c2 - (lam * c1 + (1 - lam) * c3)
}
