package m4f.ch07

import kotlin.math.exp
import kotlin.math.max

// Sections 7.1-7.3: option payoffs, put-call parity and bounds on option prices.
// Continuous compounding at rate r; T is the time to expiry in years and X the
// strike. div is the present value of dividends paid before T.

// 7.1 Definitions
fun callPayoff(S: Double, X: Double): Double = max(S - X, 0.0)
fun putPayoff(S: Double, X: Double): Double = max(X - S, 0.0)

/** Profit at T of a long position bought for `premium` at 0 (premium financed
 *  at r); a short position earns the negative of it. */
fun profit(payoff: Double, premium: Double, r: Double, T: Double): Double = payoff - premium * exp(r * T)

/** 7.2 Put-call parity: C - P = S(0) - div - X e^(-rT).
 *  Returns C - P - (S(0) - div - X e^(-rT)). Positive: write the call, buy the put
 *  and the stock, borrow; negative: the reverse. Either locks in |gap| e^(rT) at T. */
fun parityGap(C: Double, P: Double, S0: Double, X: Double, r: Double, T: Double, div: Double = 0.0): Double =
    C - P - (S0 - div - X * exp(-r * T))

data class Bounds(val lo: Double, val hi: Double)

// 7.3.1 European options
/** max(0, S(0) - div - X e^(-rT)) <= C_E <= S(0) - div */
fun europeanCallBounds(S0: Double, X: Double, r: Double, T: Double, div: Double = 0.0): Bounds =
    Bounds(max(0.0, S0 - div - X * exp(-r * T)), S0 - div)

/** max(0, X e^(-rT) - S(0) + div) <= P_E <= X e^(-rT) */
fun europeanPutBounds(S0: Double, X: Double, r: Double, T: Double, div: Double = 0.0): Bounds =
    Bounds(max(0.0, X * exp(-r * T) - S0 + div), X * exp(-r * T))

/** How far a quoted price lies outside [lo, hi] (0 if inside); this much can be
 *  locked in at time 0 by trading against the quote. */
fun boundViolation(price: Double, b: Bounds): Double =
    if (price < b.lo) b.lo - price else if (price > b.hi) price - b.hi else 0.0

/** 7.3.3 American options: S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) (no dividends) */
fun americanParityBounds(S0: Double, X: Double, r: Double, T: Double): Bounds = Bounds(S0 - X, S0 - X * exp(-r * T))

/** 7.4.1 Dependence on the strike, for X1 < X2 < X3. Convexity in X:
 *  C(X2) <= l C(X1) + (1-l) C(X3), l = (X3-X2)/(X3-X1). A positive return value is
 *  the arbitrage profit from writing the middle option and buying the wings. */
fun butterflyGap(C1: Double, C2: Double, C3: Double, X1: Double, X2: Double, X3: Double): Double {
    val lam = (X3 - X2) / (X3 - X1)
    return C2 - (lam * C1 + (1 - lam) * C3)
}
