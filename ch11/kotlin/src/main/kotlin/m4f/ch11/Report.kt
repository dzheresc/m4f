package m4f.ch11

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.exp
import kotlin.math.max

// Round-half-even on the exact binary value, like C++ std::format and Python, so the report is
// byte-identical across the three implementations. A tiny negative that rounds to zero prints as
// "-0.0000" there, so it does here too.
private fun fmt(v: Double): String {
    val s = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()
    return if (v < 0 && !s.startsWith("-")) "-$s" else s
}

/** "[a, b, c]" with each entry formatted by [fmt]. */
private fun fmtList(values: List<Double>): String = values.joinToString(", ", "[", "]") { fmt(it) }

private fun StringBuilder.line(s: String) {
    append(s).append('\n')
}

// The tree of Section 11.1: r(n,k) = 0.05 + 0.01 k - 0.01 (n-k), tau = 1.
private const val NUM_STEPS = 4
private const val TAU = 1.0

/** Section 11.1. */
private fun StringBuilder.binomialTreeSection(rates: Lattice) {
    line("\n== 11.1 Binomial tree model: r(n,k) = 0.05 + 0.01 k - 0.01 (n-k), tau=1, N=4 ==")
    for (n in 0..NUM_STEPS) {
        val row = rates[n].reversed()
        val oneStep = row.map { oneStepPrice(it, TAU) }
        line("n=$n: r=${fmtList(row)} B(n,n+1)=${fmtList(oneStep)}")
    }
    for (path in listOf("uuuu", "udud", "dddd")) {
        line("Money market account along $path: A=${fmtList(moneyMarket(rates, TAU, path))}")
    }
}

/** Sections 11.2 and 11.2.1. */
private fun StringBuilder.bondPricingSection(rates: Lattice, probs: Lattice) {
    line("\n== 11.2 Arbitrage pricing of bonds with p*=0.5 ==")
    val b0 = bondPrices(rates, probs, TAU, NUM_STEPS)
    line("B(0,N) for N=1..4: " + fmtList(b0))
    val yields = b0.withIndex().map { (m, b) -> yieldRate(b, 0, m + 1, TAU) }
    line("Term structure y(0,N): " + fmtList(yields))
    val b4 = bondLattice(rates, probs, TAU, NUM_STEPS)
    for (n in 0..NUM_STEPS) line("B(n,4) at n=$n: ${fmtList(b4[n].reversed())}")
    val b3 = bondLattice(rates, probs, TAU, 3)
    val b2 = bondLattice(rates, probs, TAU, 2)
    val lattices = listOf(2 to b2, 3 to b3, 4 to b4)
    val yUp = lattices.map { (m, b) -> yieldRate(b[1][1], 1, m, TAU) }
    val yDown = lattices.map { (m, b) -> yieldRate(b[1][0], 1, m, TAU) }
    line(
        "Term structure after one step: up node y(1,N)=${fmtList(yUp)} " +
            "down node y(1,N)=${fmtList(yDown)}",
    )
    val returns = lattices.map { (_, b) -> (0.5 * b[1][1] + 0.5 * b[1][0]) / b[0][0] }
    line(
        "Risk-neutral one-step return E*[B(1,N)]/B(0,N) for N=2,3,4: " + fmtList(returns) +
            " = e^(r(0) tau) = ${fmt(exp(0.05 * TAU))}",
    )

    line("\n== 11.2.1 Risk-neutral probabilities implied by bond prices ==")
    for ((m, b) in listOf(4 to b4, 3 to b3, 2 to b2)) {
        val implied = impliedProbs(b, rates, TAU)
        val levels =
            implied.indices.joinToString(" ") { n -> "n=$n:${fmtList(implied[n].reversed())}" }
        line("from B(n,$m): $levels")
    }
    val probs6 = constantProbs(0.6, NUM_STEPS)
    val b4p6 = bondLattice(rates, probs6, TAU, NUM_STEPS)
    line(
        "Bonds priced with p*=0.6: B(0,4)=${fmt(b4p6[0][0])} (vs ${fmt(b4[0][0])}); " +
            "implied p* at (0,0): ${fmt(impliedProb(b4p6, rates, TAU, 0, 0))}",
    )
    val wrong = b2.map { it.toMutableList() }
    wrong[0][0] = b2[0][0] - 0.01
    line(
        "A two-year bond quoted at ${fmt(wrong[0][0])} instead of ${fmt(b2[0][0])} " +
            "implies p*=${fmt(impliedProb(wrong, rates, TAU, 0, 0))} " +
            "at (0,0), inconsistent with the other bonds: arbitrage",
    )
}

/** Section 11.3.1. */
private fun StringBuilder.bondOptionsSection(rates: Lattice, probs: Lattice) {
    line("\n== 11.3.1 Options on the 4-year bond, expiry n=2, p*=0.5 ==")
    val strike = 0.90
    for ((name, isCall) in listOf("call" to true, "put" to false)) {
        val v = bondOption(rates, probs, TAU, NUM_STEPS, 2, strike, isCall)
        line(
            "European $name X=${fmt(strike)}: payoff at n=2 ${fmtList(v[2].reversed())}, " +
                "price ${fmt(v[0][0])}",
        )
    }
    val american = bondOption(rates, probs, TAU, NUM_STEPS, 2, strike, false, american = true)
    val b4 = bondLattice(rates, probs, TAU, NUM_STEPS)
    val exercise = b4[1].reversed().map { max(strike - it, 0.0) }
    line(
        "American put X=${fmt(strike)}: price ${fmt(american[0][0])} = immediate exercise " +
            "X - B(0,4) = ${fmt(strike - b4[0][0])}; values at n=1 " +
            "${fmtList(american[1].reversed())} = exercise values ${fmtList(exercise)}",
    )
    val americanCall =
        bondOption(rates, probs, TAU, NUM_STEPS, 3, 0.85, true, american = true)[0][0]
    val europeanCall = bondOption(rates, probs, TAU, NUM_STEPS, 3, 0.85, true)[0][0]
    line(
        "American call X=0.85 on the 4-year bond, expiry n=3: " +
            "${fmt(americanCall)} vs European ${fmt(europeanCall)}",
    )
    val shortCall = bondOption(rates, probs, TAU, 2, 1, 0.95, true)[0][0]
    line("Call on the 2-year bond expiring at n=1, X=0.95: " + fmt(shortCall))
}

/** Sections 11.3.2 and 11.3.3. */
private fun StringBuilder.swapsCapsFloorsSection(rates: Lattice, probs: Lattice) {
    line(
        "\n== 11.3.2 Swaps: fixed K against floating L(n,k) = e^(r tau) - 1, " +
            "4 annual payments ==",
    )
    for (n in 0 until NUM_STEPS) {
        val floating = (n downTo 0).map { k -> floatingRate(rates, TAU, n, k) }
        line("n=$n: L=${fmtList(floating)}")
    }
    val swapRate = swapRate(bondPrices(rates, probs, TAU, NUM_STEPS))
    line("Swap rate K = (1 - B(0,4)) / sum B(0,n) = ${fmt(swapRate)}")
    val swap = payerSwap(rates, probs, TAU, NUM_STEPS, swapRate)
    line("Payer swap value at K: ${fmt(swap[0][0])}; at n=1: ${fmtList(swap[1].reversed())}")
    for (fixedRate in listOf(0.04, 0.05, 0.06)) {
        val value = payerSwap(rates, probs, TAU, NUM_STEPS, fixedRate)[0][0]
        line("Payer swap at K=${fmt(fixedRate)}: ${fmt(value)}")
    }

    line("\n== 11.3.3 Caps and floors on the same floating rate ==")
    for (strike in listOf(0.04, swapRate, 0.06)) {
        val c = cap(rates, probs, TAU, NUM_STEPS, strike)[0][0]
        val fl = floor(rates, probs, TAU, NUM_STEPS, strike)[0][0]
        val s = payerSwap(rates, probs, TAU, NUM_STEPS, strike)[0][0]
        line(
            "K=${fmt(strike)}: cap=${fmt(c)} floor=${fmt(fl)} cap-floor=${fmt(c - fl)} " +
                "= payer swap ${fmt(s)}",
        )
    }
    val caplets = (0 until NUM_STEPS).map { caplet(rates, probs, TAU, it, 0.05) }
    val totalCap = cap(rates, probs, TAU, NUM_STEPS, 0.05)[0][0]
    line(
        "Caplets at K=0.05 by fixing date n=0..3: ${fmtList(caplets)}, " +
            "sum ${fmt(plainSum(caplets))} = cap ${fmt(totalCap)}",
    )
}

/** The full chapter 11 report. */
fun report(): String =
    buildString {
        line("Chapter 11: Stochastic Interest Rates (illustrative numbers, book formulas)")
        val rates = rateLattice(0.05, 0.01, -0.01, NUM_STEPS)
        val probs = constantProbs(0.5, NUM_STEPS)
        binomialTreeSection(rates)
        bondPricingSection(rates, probs)
        bondOptionsSection(rates, probs)
        swapsCapsFloorsSection(rates, probs)
    }
