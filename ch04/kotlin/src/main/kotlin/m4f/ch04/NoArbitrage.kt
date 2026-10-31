// Sections 4.1.2-4.1.4: no arbitrage, the binomial tree and the Fundamental Theorem of Asset
// Pricing.
//
// A one-step market has one stock whose return over the step is one of the scenario returns K_i,
// and a money market returning r. It admits no arbitrage exactly when a risk-neutral probability
// exists: q_i > 0 with sum q_i K_i = r, which happens exactly when min K_i < r < max K_i.
//
// Names follow the book: s0 = S(0) and a0 = A(0) are the stock and money market prices at time
// 0, v0 = V(0) is the initial wealth, u and d are the up and down returns of the binomial tree, p
// is the probability of an up move, and x, y are the numbers of shares and money market units held.
package m4f.ch04

import kotlin.math.min
import kotlin.math.pow

fun isArbitrageFree(returns: List<Double>, r: Double): Boolean =
    returns.min() < r && r < returns.max()

/**
 * (x, y) with V(0) = x S(0) + y A(0) = 0 and V(1) >= 0 in every scenario, > 0 in at least one;
 * null when the market is arbitrage-free.
 */
fun arbitrageStrategy(
    returns: List<Double>,
    r: Double,
    s0: Double,
    a0: Double,
): Pair<Double, Double>? = when {
    r >= returns.max() -> Pair(-1.0, s0 / a0)
    r <= returns.min() -> Pair(1.0, -s0 / a0)
    else -> null
}

/** V(1) = x S(1) + y A(1) in each scenario. */
fun oneStepValues(
    x: Double,
    y: Double,
    returns: List<Double>,
    r: Double,
    s0: Double,
    a0: Double,
): List<Double> = returns.map { x * s0 * (1 + it) + y * a0 * (1 + r) }

/**
 * A strictly positive q with sum q_i K_i = r, or null when there is none. With two scenarios it
 * is the unique p*; with more, one of many: a mix of the uniform distribution (weight w) and a
 * two-point law on the lowest and highest return.
 */
fun riskNeutralProbs(returns: List<Double>, r: Double): List<Double>? {
    if (!isArbitrageFree(returns, r)) return null
    val n = returns.size
    val hi = returns.max()
    val lo = returns.min()
    val iHi = returns.indexOf(hi)
    val iLo = returns.indexOf(lo)
    var sum = 0.0
    for (k in returns) {
        sum += k
    }
    val mean = sum / n
    var w = 1.0
    if (mean > lo) w = min(w, (r - lo) / (mean - lo))
    if (hi > mean) w = min(w, (hi - r) / (hi - mean))
    w = if (n > 2) 0.5 * w else 0.0
    val target = (r - w * mean) / (1 - w)
    val p = (target - lo) / (hi - lo)
    val q = MutableList(n) { w / n }
    q[iHi] += (1 - w) * p
    q[iLo] += (1 - w) * (1 - p)
    return q
}

/**
 * 4.1.3 Binomial tree: a strategy is a rule x(n, moves) giving the shares held over (n-1, n] from
 * the moves made so far (1 = up, 0 = down; moves.size = n-1).
 */
typealias Rule = (Int, List<Int>) -> Double

/**
 * E(V~(N)) over all 2^N paths, with V~(N) = V(N) / (1+r)^N and A(0) = 1. Under p = p* this equals
 * V(0) for every self-financing rule.
 */
fun expectedDiscountedValue(
    s0: Double,
    u: Double,
    d: Double,
    r: Double,
    numSteps: Int,
    v0: Double,
    rule: Rule,
    p: Double,
): Double {
    val a = (0..numSteps).map { (1 + r).pow(it) }
    var total = 0.0
    for (mask in 0 until (1 shl numSteps)) {
        val moves = (0 until numSteps).map { (mask shr it) and 1 }
        val s = mutableListOf(s0)
        for (move in moves) {
            s.add(s.last() * (1 + (if (move == 1) u else d)))
        }
        val x = (1..numSteps).map { n -> rule(n, moves.subList(0, n - 1)) }
        val values = selfFinancing(v0, s, a, x).values
        val ups = moves.sum()
        total += p.pow(ups) * (1 - p).pow(numSteps - ups) * values[numSteps] / a[numSteps]
    }
    return total
}
