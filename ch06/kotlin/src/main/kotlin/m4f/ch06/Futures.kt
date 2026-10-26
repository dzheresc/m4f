package m4f.ch06

import kotlin.math.pow
import kotlin.math.sqrt

// Section 6.2, futures: pricing in the binomial model, marking to market, and
// hedging with futures.

/** 6.2.1 Pricing. In the binomial tree (S0, u, d, r per step) the futures price
 *  for delivery at N is f(n) = E*(S(N) | S(n)): f(N) = S(N) and backwards
 *  f(n, k) = p* f(n+1, k+1) + (1 - p*) f(n+1, k). Returns f[n][k] after k up
 *  moves out of n. */
fun futuresTree(S0: Double, u: Double, d: Double, r: Double, N: Int): List<List<Double>> {
    val q = (r - d) / (u - d)
    val f = MutableList(N + 1) { emptyList<Double>() }
    f[N] = (0..N).map { k -> S0 * (1 + u).pow(k) * (1 + d).pow(N - k) }
    for (n in N - 1 downTo 0) {
        val nxt = f[n + 1]
        f[n] = (0..n).map { k -> q * nxt[k + 1] + (1 - q) * nxt[k] }
    }
    return f
}

/** balance: after settlement, before any call; call: restores the initial margin */
data class MarginDay(val balance: Double, val call: Double)

/** Marking to market for one long contract on `size` units: each day the account
 *  gains size * (f(n) - f(n-1)); when it falls below the maintenance margin, a
 *  call restores it to the initial margin. */
fun marginAccount(prices: List<Double>, size: Double, initial: Double, maintenance: Double): List<MarginDay> {
    var balance = initial
    val out = mutableListOf<MarginDay>()
    for (n in 1 until prices.size) {
        balance += size * (prices[n] - prices[n - 1])
        val call = if (balance < maintenance) initial - balance else 0.0
        out.add(MarginDay(balance, call))
        balance += call
    }
    return out
}

// 6.2.2 Hedging with futures. Selling h futures per unit of spot exposure turns
// a spot change dS into dS - h df.
/** h* = Cov(dS, df) / Var(df) */
fun minVarianceHedgeRatio(dS: List<Double>, df: List<Double>, p: List<Double>): Double {
    var mS = 0.0
    var mf = 0.0
    for (i in p.indices) {
        mS += p[i] * dS[i]
        mf += p[i] * df[i]
    }
    var cov = 0.0
    var variance = 0.0
    for (i in p.indices) {
        cov += p[i] * (dS[i] - mS) * (df[i] - mf)
        variance += p[i] * (df[i] - mf) * (df[i] - mf)
    }
    return cov / variance
}

/** Standard deviation of dS - h df */
fun hedgedStd(h: Double, dS: List<Double>, df: List<Double>, p: List<Double>): Double {
    val x = dS.indices.map { dS[it] - h * df[it] }
    var m = 0.0
    for (i in x.indices) m += p[i] * x[i]
    var v = 0.0
    for (i in x.indices) v += p[i] * (x[i] - m) * (x[i] - m)
    return sqrt(v)
}
