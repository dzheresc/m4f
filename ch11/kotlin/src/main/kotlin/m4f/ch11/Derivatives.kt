package m4f.ch11

import kotlin.math.max

// Section 11.3: interest rate derivatives on the binomial tree.
//
// Floating payments are the simple one-period rate fixed at (n,k) and paid at n+1:
// L(n,k) = 1/B(n,n+1;k) - 1 = e^(r(n,k) tau) - 1, on a notional of 1.

/** Left-to-right sum, matching the C++ and Python loops bit for bit. */
fun plainSum(xs: List<Double>): Double {
    var t = 0.0
    for (x in xs) t += x
    return t
}

/** L(n,k) = 1/B(n,n+1;k) - 1. */
fun floatingRate(rates: Lattice, tau: Double, n: Int, k: Int): Double =
    1 / oneStepPrice(rates[n][k], tau) - 1

/** 11.3.1 Options on a zero-coupon bond maturing at [maturity], expiring at [expiry]. */
fun bondOption(
    rates: Lattice,
    probs: Lattice,
    tau: Double,
    maturity: Int,
    expiry: Int,
    strike: Double,
    isCall: Boolean,
    american: Boolean = false,
): Lattice {
    val bond = bondLattice(rates, probs, tau, maturity)
    fun payoff(b: Double) = if (isCall) max(b - strike, 0.0) else max(strike - b, 0.0)
    val terminal = (0..expiry).map { payoff(bond[expiry][it]) }
    val early: NodeFn? = if (american) { n, k -> payoff(bond[n][k]) } else null
    return discountBack(rates, probs, tau, terminal, null, early)
}

/**
 * 11.3.2 Swaps: fixed K against floating L(n,k), N payments at 1..N on notional 1. The fixed rate
 * making the swap worthless: K sum B(0,n) = 1 - B(0,N).
 */
fun swapRate(bondPrices: List<Double>): Double = (1 - bondPrices.last()) / plainSum(bondPrices)

/** Value lattice of a stream paying payment(n,k) at n+1 for n = 0..N-1. */
fun pricePayments(
    rates: Lattice,
    probs: Lattice,
    tau: Double,
    numSteps: Int,
    payment: NodeFn,
): Lattice = discountBack(rates, probs, tau, List(numSteps + 1) { 0.0 }, payment)

/** Receive floating, pay fixed [fixedRate]. */
fun payerSwap(
    rates: Lattice,
    probs: Lattice,
    tau: Double,
    numSteps: Int,
    fixedRate: Double,
): Lattice =
    pricePayments(rates, probs, tau, numSteps) { n, k ->
        floatingRate(rates, tau, n, k) - fixedRate
    }

/** 11.3.3 A cap: each caplet pays max(L(n,k) - K, 0). */
fun cap(rates: Lattice, probs: Lattice, tau: Double, numSteps: Int, strike: Double): Lattice =
    pricePayments(rates, probs, tau, numSteps) { n, k ->
        max(floatingRate(rates, tau, n, k) - strike, 0.0)
    }

/** 11.3.3 A floor: each floorlet pays max(K - L(n,k), 0). */
fun floor(rates: Lattice, probs: Lattice, tau: Double, numSteps: Int, strike: Double): Lattice =
    pricePayments(rates, probs, tau, numSteps) { n, k ->
        max(strike - floatingRate(rates, tau, n, k), 0.0)
    }

/**
 * Value at 0 of the single caplet fixed at [fixing] and paid at fixing + 1. Only level [fixing]
 * pays, so this rolls one row back from there instead of building the whole pricePayments()
 * lattice; the result is the same.
 */
fun caplet(rates: Lattice, probs: Lattice, tau: Double, fixing: Int, strike: Double): Double {
    val v = DoubleArray(fixing + 1) { k ->
        oneStepPrice(rates[fixing][k], tau) * max(floatingRate(rates, tau, fixing, k) - strike, 0.0)
    }
    for (n in fixing - 1 downTo 0) {
        for (k in 0..n) {
            val p = probs[n][k]
            v[k] = oneStepPrice(rates[n][k], tau) * (p * v[k + 1] + (1 - p) * v[k])
        }
    }
    return v[0]
}
