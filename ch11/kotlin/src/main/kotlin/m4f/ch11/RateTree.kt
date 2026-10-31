package m4f.ch11

import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.max

// Sections 11.1-11.2: the binomial tree model of the short rate.
//
// Time steps of length tau; node (n,k) is reached after n steps of which k were "up". A Lattice
// holds one value per node: L[n][k], k = 0..n. The short rate r(n,k) applies over [n, n+1], so the
// one-step bond is B(n,n+1;k) = e^(-r(n,k) tau) and the money market account grows by e^(r tau)
// each step. Risk-neutral probabilities p*(n,k) of an up move may vary by node.

typealias Lattice = List<List<Double>>

/** A value attached to node (n, k). */
typealias NodeFn = (Int, Int) -> Double

/** Additive tree: r(n,k) = r0 + k up + (n-k) down (down is negative for a fall). */
fun rateLattice(r0: Double, up: Double, down: Double, numSteps: Int): Lattice =
    List(numSteps + 1) { n -> List(n + 1) { k -> r0 + k * up + (n - k) * down } }

/** The same up probability [p] at every node of levels 0..numSteps-1. */
fun constantProbs(p: Double, numSteps: Int): Lattice = List(numSteps) { n -> List(n + 1) { p } }

/** B(n,n+1) = e^(-r tau). */
fun oneStepPrice(r: Double, tau: Double): Double = exp(-r * tau)

/** A(0..len(path)) along a [path] of 'u'/'d' moves: A(n+1) = A(n) e^(r(n,k) tau). */
fun moneyMarket(rates: Lattice, tau: Double, path: String): List<Double> {
    val account = arrayListOf(1.0)
    var k = 0
    for ((n, move) in path.withIndex()) {
        account.add(account.last() * exp(rates[n][k] * tau))
        if (move == 'u') k += 1
    }
    return account
}

/**
 * Backward induction from level N = terminal.size-1 down to 0:
 * V(n,k) = e^(-r(n,k) tau) [ p* V(n+1,k+1) + (1-p*) V(n+1,k) + payment(n,k) ] with the payment
 * (if any) made at n+1 but fixed at (n,k), and, if [early] is given,
 * V(n,k) = max(V(n,k), early(n,k)) (American exercise).
 */
fun discountBack(
    rates: Lattice,
    probs: Lattice,
    tau: Double,
    terminal: List<Double>,
    payment: NodeFn? = null,
    early: NodeFn? = null,
): Lattice {
    val numSteps = terminal.size - 1
    val values = List(numSteps + 1) { n -> DoubleArray(n + 1) }
    for (k in 0..numSteps) values[numSteps][k] = terminal[k]
    for (n in numSteps - 1 downTo 0) {
        for (k in 0..n) {
            val p = probs[n][k]
            var cont = p * values[n + 1][k + 1] + (1 - p) * values[n + 1][k]
            if (payment != null) cont += payment(n, k)
            var v = oneStepPrice(rates[n][k], tau) * cont
            if (early != null) v = max(v, early(n, k))
            values[n][k] = v
        }
    }
    return values.map { it.toList() }
}

/** 11.2: B(n,N;k) for all nodes, from B(N,N) = 1. */
fun bondLattice(rates: Lattice, probs: Lattice, tau: Double, maturity: Int): Lattice =
    discountBack(rates, probs, tau, List(maturity + 1) { 1.0 })

/** B(0,1), ..., B(0,N). */
fun bondPrices(rates: Lattice, probs: Lattice, tau: Double, numSteps: Int): List<Double> =
    (1..numSteps).map { bondLattice(rates, probs, tau, it)[0][0] }

/** y(n,N) = -ln B(n,N) / ((N-n) tau). */
fun yieldRate(bondPrice: Double, n: Int, maturity: Int, tau: Double): Double =
    -ln(bondPrice) / ((maturity - n) * tau)

/**
 * 11.2.1: the p*(n,k) that prices the bond with lattice [bond] at node (n,k):
 * B(n,N;k) e^(r tau) = p* B(n+1,N;k+1) + (1-p*) B(n+1,N;k).
 */
fun impliedProb(bond: Lattice, rates: Lattice, tau: Double, n: Int, k: Int): Double =
    (bond[n][k] * exp(rates[n][k] * tau) - bond[n + 1][k]) /
        (bond[n + 1][k + 1] - bond[n + 1][k])

/** [impliedProb] at every node of levels 0..N-2 of [bond]. */
fun impliedProbs(bond: Lattice, rates: Lattice, tau: Double): Lattice {
    val maturity = bond.size - 1
    return List(maturity - 1) { n -> List(n + 1) { k -> impliedProb(bond, rates, tau, n, k) } }
}
