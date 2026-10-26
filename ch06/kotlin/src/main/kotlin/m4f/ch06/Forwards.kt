package m4f.ch06

import kotlin.math.abs
import kotlin.math.exp

// Section 6.1, forward contracts, with continuous compounding at rate r.
// Times are in years; F(t, T) is the forward price at t for delivery at T.

// 6.1.1 Forward price
/** F(t, T) = S(t) e^(r (T - t)), tau = T - t */
fun forwardPrice(S: Double, r: Double, tau: Double): Double = S * exp(r * tau)

/** Dividends (amount, time) paid before T: F(0, T) = (S(0) - PV(div)) e^(r T) */
fun forwardPriceDividends(S0: Double, r: Double, T: Double, dividends: List<Pair<Double, Double>>): Double {
    var pv = 0.0
    for ((amount, t) in dividends) pv += amount * exp(-r * t)
    return (S0 - pv) * exp(r * T)
}

/** Continuous dividend yield rDiv (or a foreign interest rate for currencies):
 *  F(0, T) = S(0) e^((r - rDiv) T) */
fun forwardPriceYield(S0: Double, r: Double, rDiv: Double, T: Double): Double = S0 * exp((r - rDiv) * T)

/** Riskless profit at T per unit from a mispriced forward. If FQuoted is above
 *  S(0) e^(rT): borrow S(0), buy the stock, sell forward (cash and carry); if
 *  below: short the stock, invest S(0), buy forward. Either way the profit is
 *  |FQuoted - S(0) e^(rT)|. */
fun forwardArbitrageProfit(FQuoted: Double, S0: Double, r: Double, T: Double): Double =
    abs(FQuoted - forwardPrice(S0, r, T))

/** 6.1.2 Value at t of a long forward entered at 0: V(t) = (F(t,T) - F(0,T)) e^(-r (T-t)) */
fun longForwardValue(F0: Double, Ft: Double, r: Double, tau: Double): Double = (Ft - F0) * exp(-r * tau)
