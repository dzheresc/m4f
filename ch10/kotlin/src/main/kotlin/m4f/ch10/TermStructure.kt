package m4f.ch10

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.max

// Section 10.2: general term structure. The market gives zero-coupon bond prices B(0,T) for a grid
// of maturities T_1 < ... < T_n; all rates are continuously compounded. In a deterministic term
// structure the implied forward rates are the future short rates, and B(t,T) = B(0,T)/B(0,t).

/** y(0,T) = -ln B(0,T) / T. */
fun yieldRate(bondPrice: Double, maturity: Double): Double = -ln(bondPrice) / maturity

/**
 * f(0,T1,T2) = (ln B(0,T1) - ln B(0,T2)) / (T2 - T1), with [price1] = B(0,T1), [maturity1] = T1,
 * [price2] = B(0,T2), [maturity2] = T2. B(0,0) = 1 gives y(0,T2).
 */
fun forwardRate(price1: Double, maturity1: Double, price2: Double, maturity2: Double): Double =
    (ln(price1) - ln(price2)) / (maturity2 - maturity1)

/** f(0,T_{i-1},T_i) for each i, with T_0 = 0. */
fun forwardRates(maturities: List<Double>, prices: List<Double>): List<Double> {
    val out = ArrayList<Double>()
    var prevPrice = 1.0
    var prevMaturity = 0.0
    for (i in maturities.indices) {
        out.add(forwardRate(prevPrice, prevMaturity, prices[i], maturities[i]))
        prevPrice = prices[i]
        prevMaturity = maturities[i]
    }
    return out
}

/** B(0,T_n) = exp(-sum_{i<=n} f(0,T_{i-1},T_i)(T_i - T_{i-1})). */
fun pricesFromForwardRates(maturities: List<Double>, forwards: List<Double>): List<Double> {
    val out = ArrayList<Double>()
    var acc = 0.0
    var prevMaturity = 0.0
    for (i in maturities.indices) {
        acc += forwards[i] * (maturities[i] - prevMaturity)
        out.add(exp(-acc))
        prevMaturity = maturities[i]
    }
    return out
}

/**
 * B(t,T) implied by no arbitrage in a deterministic term structure: B(0,T)/B(0,t), with [priceT] =
 * B(0,t) and [priceMaturity] = B(0,T).
 */
fun impliedPrice(priceT: Double, priceMaturity: Double): Double = priceMaturity / priceT

/**
 * Price of a coupon bond = sum C_i B(0,t_i), with t_i on the grid (matched exactly). The
 * maturities are ascending, so sorted flow times are matched in one merge pass; a flow time
 * earlier than the one before restarts the scan.
 */
fun couponBondPrice(
    flowTimes: List<Double>,
    flowAmounts: List<Double>,
    maturities: List<Double>,
    prices: List<Double>,
): Double {
    var total = 0.0
    var j = 0
    for (i in flowTimes.indices) {
        val t = flowTimes[i]
        if (i > 0 && t < flowTimes[i - 1]) j = 0
        while (j < maturities.size && maturities[j] < t) j++
        if (j < maturities.size && maturities[j] == t) total += flowAmounts[i] * prices[j]
    }
    return total
}

/**
 * The single continuous yield y in [-1, 1] with sum C_i e^(-y t_i) = [bondPrice]. Newton's method
 * (the derivative is minus the duration sum, from the same loop) inside a bisection bracket: a
 * step that leaves the bracket is replaced by its midpoint, so it converges wherever bisection
 * would.
 */
fun yieldToMaturity(bondPrice: Double, flowTimes: List<Double>, flowAmounts: List<Double>): Double {
    var lo = -1.0
    var hi = 1.0
    var total = 0.0
    var weighted = 0.0
    for (i in flowTimes.indices) {
        total += flowAmounts[i]
        weighted += flowAmounts[i] * flowTimes[i]
    }
    // Starting guess: the yield if all flows were paid at their mean time.
    var y = if (total > 0 && bondPrice > 0 && weighted > 0) {
        ln(total / bondPrice) * total / weighted
    } else {
        0.0
    }
    if (!(y > lo && y < hi)) y = 0.0
    repeat(200) {
        var s = 0.0
        var ds = 0.0 // -d s / dy.
        for (i in flowTimes.indices) {
            val term = flowAmounts[i] * exp(-y * flowTimes[i])
            s += term
            ds += flowTimes[i] * term
        }
        if (s > bondPrice) lo = y else hi = y
        if (ds > 0) {
            val step = (s - bondPrice) / ds
            // Converged: y is the root to within rounding, and may sit on a bracket end, so test
            // this before the bracket.
            if (abs(step) <= 1e-15 * max(1.0, abs(y))) return y + step
            y += step
        }
        if (!(y > lo && y < hi)) y = (lo + hi) / 2
        if (hi - lo <= 1e-15) return y
    }
    return y
}

/**
 * 10.2.2: A(T_n) from rolling over one-period bonds at the (implied, hence realised) short rates:
 * A(T_n) = prod 1/B(T_{i-1},T_i) = 1/B(0,T_n).
 */
@Suppress("UNUSED_PARAMETER")
fun moneyMarketAccount(maturities: List<Double>, prices: List<Double>): List<Double> {
    val out = ArrayList<Double>()
    var account = 1.0
    var prevPrice = 1.0
    for (p in prices) {
        account = account / impliedPrice(prevPrice, p)
        out.add(account)
        prevPrice = p
    }
    return out
}
