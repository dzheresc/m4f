package m4f.ch11

import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.max

// Sections 11.1-11.2: the binomial tree model of the short rate.
//
// Time steps of length tau; node (n,k) is reached after n steps of which k were
// "up". A Lattice holds one value per node: L[n][k], k = 0..n. The short rate
// r(n,k) applies over [n, n+1], so the one-step bond is B(n,n+1;k) = e^(-r(n,k) tau)
// and the money market account grows by e^(r tau) each step. Risk-neutral
// probabilities p*(n,k) of an up move may vary by node.

typealias Lattice = List<List<Double>>
typealias NodeFn = (Int, Int) -> Double

/** Additive tree: r(n,k) = r0 + k up + (n-k) down (down is negative for a fall). */
fun rateLattice(r0: Double, up: Double, down: Double, N: Int): Lattice =
    List(N + 1) { n -> List(n + 1) { k -> r0 + k * up + (n - k) * down } }

fun constantProbs(p: Double, N: Int): Lattice = List(N) { n -> List(n + 1) { p } }

/** B(n,n+1) = e^(-r tau) */
fun oneStepPrice(r: Double, tau: Double): Double = exp(-r * tau)

/** A(0..len(path)) along a path of 'u'/'d' moves: A(n+1) = A(n) e^(r(n,k) tau). */
fun moneyMarket(rates: Lattice, tau: Double, path: String): List<Double> {
    val A = arrayListOf(1.0)
    var k = 0
    for ((n, move) in path.withIndex()) {
        A.add(A.last() * exp(rates[n][k] * tau))
        if (move == 'u') k += 1
    }
    return A
}

/** Backward induction from level N = terminal.size-1 down to 0:
 *  V(n,k) = e^(-r(n,k) tau) [ p* V(n+1,k+1) + (1-p*) V(n+1,k) + payment(n,k) ]
 *  with the payment (if any) made at n+1 but fixed at (n,k), and, if `early`
 *  is given, V(n,k) = max(V(n,k), early(n,k)) (American exercise). */
fun discountBack(rates: Lattice, probs: Lattice, tau: Double, terminal: List<Double>,
                 payment: NodeFn? = null, early: NodeFn? = null): Lattice {
    val N = terminal.size - 1
    val V = List(N + 1) { n -> DoubleArray(n + 1) }
    for (k in 0..N) V[N][k] = terminal[k]
    for (n in N - 1 downTo 0)
        for (k in 0..n) {
            val p = probs[n][k]
            var cont = p * V[n + 1][k + 1] + (1 - p) * V[n + 1][k]
            if (payment != null) cont += payment(n, k)
            var v = oneStepPrice(rates[n][k], tau) * cont
            if (early != null) v = max(v, early(n, k))
            V[n][k] = v
        }
    return V.map { it.toList() }
}

/** 11.2: B(n,N;k) for all nodes, from B(N,N) = 1. */
fun bondLattice(rates: Lattice, probs: Lattice, tau: Double, N: Int): Lattice =
    discountBack(rates, probs, tau, List(N + 1) { 1.0 })

/** B(0,1), ..., B(0,N) */
fun bondPrices(rates: Lattice, probs: Lattice, tau: Double, N: Int): List<Double> =
    (1..N).map { bondLattice(rates, probs, tau, it)[0][0] }

/** y(n,N) = -ln B(n,N) / ((N-n) tau) */
fun yieldRate(B: Double, n: Int, N: Int, tau: Double): Double = -ln(B) / ((N - n) * tau)

/** 11.2.1: the p*(n,k) that prices the bond with lattice B at node (n,k):
 *  B(n,N;k) e^(r tau) = p* B(n+1,N;k+1) + (1-p*) B(n+1,N;k). */
fun impliedProb(B: Lattice, rates: Lattice, tau: Double, n: Int, k: Int): Double =
    (B[n][k] * exp(rates[n][k] * tau) - B[n + 1][k]) / (B[n + 1][k + 1] - B[n + 1][k])

fun impliedProbs(B: Lattice, rates: Lattice, tau: Double): Lattice {
    val N = B.size - 1
    return List(N - 1) { n -> List(n + 1) { k -> impliedProb(B, rates, tau, n, k) } }
}
