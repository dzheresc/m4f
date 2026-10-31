// Sections 8.1-8.2: European and American options in the binomial tree model.
//
// The tree has S(n) = S(0) (1+u)^k (1+d)^(n-k) after k up moves in n steps, a money market
// growing at r per step, and p* = (r - d) / (u - d). Payoffs are functions of the final stock
// price.
package m4f.ch08

import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.ln1p
import kotlin.math.max
import kotlin.math.pow

typealias Payoff = (Double) -> Double
typealias Tree = List<List<Double>>

/** p* = (r - d) / (u - d). */
fun riskNeutralProb(u: Double, d: Double, r: Double): Double = (r - d) / (u - d)

/** S(0) (1+u)^k (1+d)^(n-k). */
fun stockPrice(s0: Double, u: Double, d: Double, n: Int, k: Int): Double =
    s0 * (1 + u).pow(k) * (1 + d).pow(n - k)

// Section 8.1.1: one step.

/**
 * A one-step replicating portfolio.
 *
 * @property x shares
 * @property y units of the money market (A(0) = 1)
 * @property price x S(0) + y
 */
data class Replication(val x: Double, val y: Double, val price: Double)

/** Replicates the payoff [cUp] / [cDown]. */
fun oneStepReplication(
    s0: Double,
    u: Double,
    d: Double,
    r: Double,
    cUp: Double,
    cDown: Double,
): Replication {
    val x = (cUp - cDown) / (s0 * (u - d))
    val y = (cDown - x * s0 * (1 + d)) / (1 + r)
    return Replication(x, y, x * s0 + y)
}

/**
 * Sections 8.1.2-8.1.3: backward induction, values[n][k] at every node.
 * European: V(n) = (p* V(n+1, up) + (1-p*) V(n+1, down)) / (1+r); American: the larger of that
 * and the payoff from exercising now.
 */
fun optionTree(
    s0: Double,
    u: Double,
    d: Double,
    r: Double,
    numSteps: Int,
    payoff: Payoff,
    american: Boolean = false,
): Tree {
    val q = riskNeutralProb(u, d, r)
    val values = MutableList(numSteps + 1) { emptyList<Double>() }
    values[numSteps] = (0..numSteps).map { k -> payoff(stockPrice(s0, u, d, numSteps, k)) }
    for (n in numSteps - 1 downTo 0) {
        values[n] = (0..n).map { k ->
            var v = (q * values[n + 1][k + 1] + (1 - q) * values[n + 1][k]) / (1 + r)
            if (american) v = max(v, payoff(stockPrice(s0, u, d, n, k)))
            v
        }
    }
    return values
}

/**
 * (x, y) held over (n, n+1] at node (n, k): x = delta of the option, y in units of the money
 * market worth (1+r)^n.
 */
fun replicatingPosition(
    s0: Double,
    u: Double,
    d: Double,
    r: Double,
    values: Tree,
    n: Int,
    k: Int,
): Pair<Double, Double> {
    val sUp = stockPrice(s0, u, d, n + 1, k + 1)
    val sDown = stockPrice(s0, u, d, n + 1, k)
    val x = (values[n + 1][k + 1] - values[n + 1][k]) / (sUp - sDown)
    val y = (values[n][k] - x * stockPrice(s0, u, d, n, k)) / (1 + r).pow(n)
    return Pair(x, y)
}

/**
 * C(N,k) p^k (1-p)^(N-k) for k = 0..N, built in log space by updating log C(N, k) from k to
 * k+1: C(N, k) on its own overflows a Double near N = 1030 even when the weight is tiny.
 */
fun binomialWeights(p: Double, numSteps: Int): DoubleArray {
    if (p <= 0 || p >= 1) {
        val hit = if (p <= 0) 0 else numSteps
        return DoubleArray(numSteps + 1) { if (it == hit) 1.0 else 0.0 }
    }
    val logP = ln(p)
    val logQ = ln1p(-p)
    var logC = 0.0
    val weights = DoubleArray(numSteps + 1)
    for (k in 0..numSteps) {
        weights[k] = exp(logC + k * logP + (numSteps - k) * logQ)
        if (k < numSteps) logC += ln((numSteps - k).toDouble()) - ln((k + 1).toDouble())
    }
    return weights
}

/** P(at least m up moves out of N) = sum_{k >= m} C(N,k) p^k (1-p)^(N-k). */
fun binomialSum(p: Double, numSteps: Int, m: Int): Double {
    val weights = binomialWeights(p, numSteps)
    var total = 0.0
    for (k in m.coerceAtLeast(0)..numSteps) total += weights[k]
    return total
}

/** Section 8.1.3: (1+r)^-N sum_k C(N,k) p*^k (1-p*)^(N-k) f(S(N, k)). */
fun europeanPriceSum(
    s0: Double,
    u: Double,
    d: Double,
    r: Double,
    numSteps: Int,
    payoff: Payoff,
): Double {
    val weights = binomialWeights(riskNeutralProb(u, d, r), numSteps)
    var total = 0.0
    for (k in 0..numSteps) total += weights[k] * payoff(stockPrice(s0, u, d, numSteps, k))
    return total * (1 + r).pow(-numSteps)
}

/**
 * Section 8.1.4: the Cox-Ross-Rubinstein formula,
 * C = S(0) Phi(m; N, p') - X (1+r)^-N Phi(m; N, p*), with m the least number of up moves for
 * which S(N) > X and p' = p* (1+u) / (1+r).
 */
data class CrrCallResult(val price: Double, val m: Int)

/** The CRR call price and m; see [CrrCallResult]. */
fun crrCall(
    s0: Double,
    strike: Double,
    u: Double,
    d: Double,
    r: Double,
    numSteps: Int,
): CrrCallResult {
    val q = riskNeutralProb(u, d, r)
    var m = 0
    while (m <= numSteps && stockPrice(s0, u, d, numSteps, m) <= strike) m++
    val qPrime = q * (1 + u) / (1 + r)
    return CrrCallResult(
        s0 * binomialSum(qPrime, numSteps, m) -
            strike * (1 + r).pow(-numSteps) * binomialSum(q, numSteps, m),
        m,
    )
}

/** Section 8.2: American options. Nodes (n, k), n < N, where exercising beats waiting. */
fun earlyExerciseNodes(
    s0: Double,
    u: Double,
    d: Double,
    r: Double,
    numSteps: Int,
    payoff: Payoff,
): List<Pair<Int, Int>> {
    val q = riskNeutralProb(u, d, r)
    val values = optionTree(s0, u, d, r, numSteps, payoff, true)
    val nodes = mutableListOf<Pair<Int, Int>>()
    for (n in 0 until numSteps) {
        for (k in 0..n) {
            val wait = (q * values[n + 1][k + 1] + (1 - q) * values[n + 1][k]) / (1 + r)
            if (payoff(stockPrice(s0, u, d, n, k)) > wait) nodes.add(Pair(n, k))
        }
    }
    return nodes
}
