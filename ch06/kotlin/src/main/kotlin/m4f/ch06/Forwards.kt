// Section 6.1, forward contracts, with continuous compounding at rate r.
// Times are in years; F(t, T) is the forward price at t for delivery at T.
package m4f.ch06

import kotlin.math.abs
import kotlin.math.exp

// Section 6.1.1: forward price.

/** F(t, T) = S(t) e^(r (T - t)), tau = T - t. */
fun forwardPrice(s: Double, r: Double, tau: Double): Double = s * exp(r * tau)

/** Dividends (amount, time) paid before T: F(0, T) = (S(0) - PV(div)) e^(r T). */
fun forwardPriceDividends(
    s0: Double,
    r: Double,
    maturity: Double,
    dividends: List<Pair<Double, Double>>,
): Double {
    var pv = 0.0
    for ((amount, t) in dividends) {
        pv += amount * exp(-r * t)
    }
    return (s0 - pv) * exp(r * maturity)
}

/**
 * Continuous dividend yield [rDiv] (or a foreign interest rate for currencies):
 * F(0, T) = S(0) e^((r - rDiv) T).
 */
fun forwardPriceYield(s0: Double, r: Double, rDiv: Double, maturity: Double): Double =
    s0 * exp((r - rDiv) * maturity)

/**
 * Riskless profit at T per unit from a mispriced forward. If [quoted] is above S(0) e^(rT):
 * borrow S(0), buy the stock, sell forward (cash and carry); if below: short the stock, invest
 * S(0), buy forward. Either way the profit is |quoted - S(0) e^(rT)|.
 */
fun forwardArbitrageProfit(quoted: Double, s0: Double, r: Double, maturity: Double): Double =
    abs(quoted - forwardPrice(s0, r, maturity))

/**
 * Section 6.1.2: value at t of a long forward entered at 0,
 * V(t) = (F(t,T) - F(0,T)) e^(-r (T-t)).
 */
fun longForwardValue(f0: Double, ft: Double, r: Double, tau: Double): Double =
    (ft - f0) * exp(-r * tau)
