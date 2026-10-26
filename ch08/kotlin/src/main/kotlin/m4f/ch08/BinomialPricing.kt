package m4f.ch08

import kotlin.math.max
import kotlin.math.pow

// Sections 8.1-8.2: European and American options in the binomial tree model.
//
// The tree has S(n) = S(0) (1+u)^k (1+d)^(n-k) after k up moves in n steps, a
// money market growing at r per step, and p* = (r - d) / (u - d). Payoffs are
// functions of the final stock price.

typealias Payoff = (Double) -> Double
typealias Tree = List<List<Double>>

fun riskNeutralProb(u: Double, d: Double, r: Double): Double = (r - d) / (u - d)

fun stockPrice(S0: Double, u: Double, d: Double, n: Int, k: Int): Double = S0 * (1 + u).pow(k) * (1 + d).pow(n - k)

// 8.1.1 One step
/** x shares, y units of the money market (A(0) = 1); price = x S(0) + y */
data class Replication(val x: Double, val y: Double, val price: Double)

/** Replicates the payoff Cu / Cd. */
fun oneStepReplication(S0: Double, u: Double, d: Double, r: Double, Cu: Double, Cd: Double): Replication {
    val x = (Cu - Cd) / (S0 * (u - d))
    val y = (Cd - x * S0 * (1 + d)) / (1 + r)
    return Replication(x, y, x * S0 + y)
}

/** 8.1.2-8.1.3 Backward induction: V[n][k], option values at every node.
 *  European: V(n) = (p* V(n+1, up) + (1-p*) V(n+1, down)) / (1+r); American: the
 *  larger of that and the payoff from exercising now. */
fun optionTree(S0: Double, u: Double, d: Double, r: Double, N: Int, payoff: Payoff, american: Boolean = false): Tree {
    val q = riskNeutralProb(u, d, r)
    val V = MutableList(N + 1) { emptyList<Double>() }
    V[N] = (0..N).map { k -> payoff(stockPrice(S0, u, d, N, k)) }
    for (n in N - 1 downTo 0)
        V[n] = (0..n).map { k ->
            var v = (q * V[n + 1][k + 1] + (1 - q) * V[n + 1][k]) / (1 + r)
            if (american) v = max(v, payoff(stockPrice(S0, u, d, n, k)))
            v
        }
    return V
}

/** (x, y) held over (n, n+1] at node (n, k): x = delta of the option, y in units
 *  of the money market worth (1+r)^n. */
fun replicatingPosition(S0: Double, u: Double, d: Double, r: Double, V: Tree, n: Int, k: Int): Pair<Double, Double> {
    val Su = stockPrice(S0, u, d, n + 1, k + 1)
    val Sd = stockPrice(S0, u, d, n + 1, k)
    val x = (V[n + 1][k + 1] - V[n + 1][k]) / (Su - Sd)
    val y = (V[n][k] - x * stockPrice(S0, u, d, n, k)) / (1 + r).pow(n)
    return Pair(x, y)
}

/** P(at least m up moves out of N) = sum_{k >= m} C(N,k) p^k (1-p)^(N-k) */
fun binomialSum(p: Double, N: Int, m: Int): Double {
    var total = 0.0
    var c = 1.0
    for (k in 0..N) {
        if (k >= m) total += c * p.pow(k) * (1 - p).pow(N - k)
        c = c * (N - k) / (k + 1)
    }
    return total
}

/** 8.1.3: (1+r)^-N sum_k C(N,k) p*^k (1-p*)^(N-k) f(S(N, k)) */
fun europeanPriceSum(S0: Double, u: Double, d: Double, r: Double, N: Int, payoff: Payoff): Double {
    val q = riskNeutralProb(u, d, r)
    var total = 0.0
    var c = 1.0
    for (k in 0..N) {
        total += c * q.pow(k) * (1 - q).pow(N - k) * payoff(stockPrice(S0, u, d, N, k))
        c = c * (N - k) / (k + 1)
    }
    return total * (1 + r).pow(-N)
}

/** 8.1.4 Cox-Ross-Rubinstein formula: C = S(0) Phi(m; N, p') - X (1+r)^-N Phi(m; N, p*),
 *  with m the least number of up moves for which S(N) > X and p' = p* (1+u) / (1+r). */
data class CrrCall(val price: Double, val m: Int)

fun crrCall(S0: Double, X: Double, u: Double, d: Double, r: Double, N: Int): CrrCall {
    val q = riskNeutralProb(u, d, r)
    var m = 0
    while (m <= N && stockPrice(S0, u, d, N, m) <= X) m++
    val qq = q * (1 + u) / (1 + r)
    return CrrCall(S0 * binomialSum(qq, N, m) - X * (1 + r).pow(-N) * binomialSum(q, N, m), m)
}

/** 8.2 American options: nodes (n, k), n < N, where exercising beats waiting */
fun earlyExerciseNodes(S0: Double, u: Double, d: Double, r: Double, N: Int, payoff: Payoff): List<Pair<Int, Int>> {
    val q = riskNeutralProb(u, d, r)
    val V = optionTree(S0, u, d, r, N, payoff, true)
    val nodes = mutableListOf<Pair<Int, Int>>()
    for (n in 0 until N)
        for (k in 0..n) {
            val wait = (q * V[n + 1][k + 1] + (1 - q) * V[n + 1][k]) / (1 + r)
            if (payoff(stockPrice(S0, u, d, n, k)) > wait) nodes.add(Pair(n, k))
        }
    return nodes
}
