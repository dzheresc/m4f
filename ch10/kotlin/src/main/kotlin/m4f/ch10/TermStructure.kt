package m4f.ch10

import kotlin.math.exp
import kotlin.math.ln

// Section 10.2: general term structure. The market gives zero-coupon bond
// prices B(0,T) for a grid of maturities T_1 < ... < T_n; all rates are
// continuously compounded. In a deterministic term structure the implied
// forward rates are the future short rates, and B(t,T) = B(0,T)/B(0,t).

/** y(0,T) = -ln B(0,T) / T */
fun yieldRate(B: Double, T: Double): Double = -ln(B) / T

/** f(0,T1,T2) = (ln B(0,T1) - ln B(0,T2)) / (T2 - T1); B(0,0) = 1 gives y(0,T2). */
fun forwardRate(B1: Double, T1: Double, B2: Double, T2: Double): Double = (ln(B1) - ln(B2)) / (T2 - T1)

/** f(0,T_{i-1},T_i) for each i, with T_0 = 0. */
fun forwardRates(maturities: List<Double>, prices: List<Double>): List<Double> {
    val out = ArrayList<Double>()
    var Bp = 1.0
    var Tp = 0.0
    for (i in maturities.indices) {
        out.add(forwardRate(Bp, Tp, prices[i], maturities[i]))
        Bp = prices[i]
        Tp = maturities[i]
    }
    return out
}

/** B(0,T_n) = exp(-sum_{i<=n} f(0,T_{i-1},T_i)(T_i - T_{i-1})) */
fun pricesFromForwardRates(maturities: List<Double>, f: List<Double>): List<Double> {
    val out = ArrayList<Double>()
    var acc = 0.0
    var Tp = 0.0
    for (i in maturities.indices) {
        acc += f[i] * (maturities[i] - Tp)
        out.add(exp(-acc))
        Tp = maturities[i]
    }
    return out
}

/** B(t,T) implied by no arbitrage in a deterministic term structure: B(0,T)/B(0,t). */
fun impliedPrice(Bt: Double, BT: Double): Double = BT / Bt

/** Price of a coupon bond = sum C_i B(0,t_i), with t_i on the grid. */
fun couponBondPrice(flowsT: List<Double>, flowsC: List<Double>, maturities: List<Double>, prices: List<Double>): Double {
    var total = 0.0
    for (i in flowsT.indices) total += flowsC[i] * prices[maturities.indexOf(flowsT[i])]
    return total
}

/** The single continuous yield y with sum C_i e^(-y t_i) = P (bisection). */
fun yieldToMaturity(P: Double, flowsT: List<Double>, flowsC: List<Double>): Double {
    var lo = -1.0
    var hi = 1.0
    repeat(200) {
        val mid = (lo + hi) / 2
        var s = 0.0
        for (i in flowsT.indices) s += flowsC[i] * exp(-mid * flowsT[i])
        if (s > P) lo = mid else hi = mid
    }
    return (lo + hi) / 2
}

/** 10.2.2: A(T_n) from rolling over one-period bonds at the (implied, hence
 *  realised) short rates: A(T_n) = prod 1/B(T_{i-1},T_i) = 1/B(0,T_n). */
@Suppress("UNUSED_PARAMETER")
fun moneyMarketAccount(maturities: List<Double>, prices: List<Double>): List<Double> {
    val out = ArrayList<Double>()
    var A = 1.0
    var Bp = 1.0
    for (B in prices) {
        A = A / impliedPrice(Bp, B)
        out.add(A)
        Bp = B
    }
    return out
}
