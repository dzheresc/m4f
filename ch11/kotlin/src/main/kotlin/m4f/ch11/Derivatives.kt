package m4f.ch11

import kotlin.math.max

// Section 11.3: interest rate derivatives on the binomial tree.
//
// Floating payments are the simple one-period rate fixed at (n,k) and paid at
// n+1: L(n,k) = 1/B(n,n+1;k) - 1 = e^(r(n,k) tau) - 1, on a notional of 1.

/** Left-to-right sum, matching the C++ and Python loops bit for bit. */
fun plainSum(xs: List<Double>): Double {
    var t = 0.0
    for (x in xs) t += x
    return t
}

fun floatingRate(rates: Lattice, tau: Double, n: Int, k: Int): Double = 1 / oneStepPrice(rates[n][k], tau) - 1

/** 11.3.1 Options on a zero-coupon bond maturing at N, expiring at nExp <= N. */
fun bondOption(rates: Lattice, probs: Lattice, tau: Double, N: Int, nExp: Int, X: Double, isCall: Boolean,
               american: Boolean = false): Lattice {
    val B = bondLattice(rates, probs, tau, N)
    val payoff = { b: Double -> if (isCall) max(b - X, 0.0) else max(X - b, 0.0) }
    val terminal = (0..nExp).map { payoff(B[nExp][it]) }
    val early: NodeFn? = if (american) { n, k -> payoff(B[n][k]) } else null
    return discountBack(rates, probs, tau, terminal, null, early)
}

/** 11.3.2 Swaps: fixed K against floating L(n,k), N payments at 1..N on notional 1.
 *  The fixed rate making the swap worthless: K sum B(0,n) = 1 - B(0,N). */
fun swapRate(B0: List<Double>): Double = (1 - B0.last()) / plainSum(B0)

/** Value lattice of a stream paying payment(n,k) at n+1 for n = 0..N-1. */
fun pricePayments(rates: Lattice, probs: Lattice, tau: Double, N: Int, payment: NodeFn): Lattice =
    discountBack(rates, probs, tau, List(N + 1) { 0.0 }, payment)

/** Receive floating, pay fixed K. */
fun payerSwap(rates: Lattice, probs: Lattice, tau: Double, N: Int, K: Double): Lattice =
    pricePayments(rates, probs, tau, N) { n, k -> floatingRate(rates, tau, n, k) - K }

/** 11.3.3 Caps and floors: caplet pays max(L(n,k) - K, 0), floorlet max(K - L(n,k), 0). */
fun cap(rates: Lattice, probs: Lattice, tau: Double, N: Int, K: Double): Lattice =
    pricePayments(rates, probs, tau, N) { n, k -> max(floatingRate(rates, tau, n, k) - K, 0.0) }

fun floor(rates: Lattice, probs: Lattice, tau: Double, N: Int, K: Double): Lattice =
    pricePayments(rates, probs, tau, N) { n, k -> max(K - floatingRate(rates, tau, n, k), 0.0) }

/** Value at 0 of the single caplet fixed at nFix and paid at nFix + 1. */
fun caplet(rates: Lattice, probs: Lattice, tau: Double, nFix: Int, K: Double): Double =
    pricePayments(rates, probs, tau, nFix + 1) { n, k ->
        if (n == nFix) max(floatingRate(rates, tau, n, k) - K, 0.0) else 0.0
    }[0][0]
