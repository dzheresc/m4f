package m4f.ch08

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.max
import kotlin.math.min

// Round-half-even on the exact binary value, like C++ std::format and Python, so the report is
// byte-identical across the three implementations. Adding 0.0 turns -0.0 into 0.0.
private fun fmt(v: Double): String =
    BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

/** "[a, b, c]" with each entry formatted by [fmt]. */
private fun fmtList(values: List<Double>): String = values.joinToString(", ", "[", "]") { fmt(it) }

private fun StringBuilder.line(s: String) {
    append(s).append('\n')
}

private fun call(strike: Double): Payoff = { s -> max(s - strike, 0.0) }

private fun put(strike: Double): Payoff = { s -> max(strike - s, 0.0) }

/** Section 8.1.1. */
private fun StringBuilder.oneStepSection() {
    line("\n== 8.1.1 One step: S(0)=100 u=0.2 d=-0.1 r=0.1, call X=100 ==")
    val rep = oneStepReplication(100.0, 0.2, -0.1, 0.1, 20.0, 0.0)
    val q = riskNeutralProb(0.2, -0.1, 0.1)
    line(
        "Replicate with x=${fmt(rep.x)} shares, y=${fmt(rep.y)} in the money market: " +
            "C(0)=${fmt(rep.price)}",
    )
    line(
        "Risk-neutral: p*=${fmt(q)}, C(0) = E*(C(1))/(1+r) = " +
            fmt((q * 20.0 + (1 - q) * 0.0) / 1.1),
    )
}

/** Section 8.1.2. */
private fun StringBuilder.twoStepsSection() {
    line("\n== 8.1.2 Two steps: same tree, call X=100 ==")
    val values = optionTree(100.0, 0.2, -0.1, 0.1, 2, call(100.0))
    for (n in 0 until 3) {
        val s = (n downTo 0).map { k -> stockPrice(100.0, 0.2, -0.1, n, k) }
        line("n=$n: S=${fmtList(s)} C=${fmtList(values[n].reversed())}")
    }
    for ((n, k) in listOf(Pair(0, 0), Pair(1, 1), Pair(1, 0))) {
        val (x, y) = replicatingPosition(100.0, 0.2, -0.1, 0.1, values, n, k)
        line("node ($n,$k): x=${fmt(x)} y=${fmt(y)}")
    }
}

/** Sections 8.1.3 and 8.1.4. */
private fun StringBuilder.generalModelSection() {
    line("\n== 8.1.3 General N-step model: N=5, X=100 ==")
    for ((name, payoff) in listOf(Pair("call", call(100.0)), Pair("put", put(100.0)))) {
        val tree = optionTree(100.0, 0.2, -0.1, 0.1, 5, payoff)[0][0]
        val sum = europeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, payoff)
        line("$name: backward induction=${fmt(tree)} sum formula=${fmt(sum)}")
    }
    // Bull spread 90/120.
    val spread: Payoff = { s -> min(max(s - 90.0, 0.0), 30.0) }
    line("bull spread 90/120: ${fmt(europeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, spread))}")

    line("\n== 8.1.4 Cox-Ross-Rubinstein formula ==")
    for (strike in listOf(80.0, 100.0, 150.0)) {
        val c = crrCall(100.0, strike, 0.2, -0.1, 0.1, 5)
        val sum = europeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, call(strike))
        line("N=5 X=${fmt(strike)}: m=${c.m} C=${fmt(c.price)} sum formula=${fmt(sum)}")
    }
}

/** Section 8.2. */
private fun StringBuilder.americanSection() {
    line("\n== 8.2 American options: S(0)=100 u=0.1 d=-0.1 r=0.02 N=4, put X=100 ==")
    val american = optionTree(100.0, 0.1, -0.1, 0.02, 4, put(100.0), true)
    val european = optionTree(100.0, 0.1, -0.1, 0.02, 4, put(100.0))
    line("P_A=${fmt(american[0][0])} P_E=${fmt(european[0][0])}")
    for (n in 0 until 5) {
        line("n=$n: P_A=${fmtList(american[n].reversed())}")
    }
    val nodes = earlyExerciseNodes(100.0, 0.1, -0.1, 0.02, 4, put(100.0))
    line(
        "Exercise early at (n,k): " +
            nodes.joinToString(", ") { (n, k) ->
                "($n,$k) S=${fmt(stockPrice(100.0, 0.1, -0.1, n, k))}"
            },
    )
}

/** Section 8.3. */
private fun StringBuilder.blackScholesSection() {
    line("\n== 8.3 Black-Scholes formula: S=100 X=100 r=0.05 sigma=0.2 T=1 ==")
    line(
        "C=${fmt(bsCall(100.0, 100.0, 0.05, 0.2, 1.0))} " +
            "P=${fmt(bsPut(100.0, 100.0, 0.05, 0.2, 1.0))} " +
            "delta=${fmt(bsCallDelta(100.0, 100.0, 0.05, 0.2, 1.0))}",
    )
    for (numSteps in listOf(10, 50, 100, 500, 1000)) {
        fun treePrice(isCall: Boolean, american: Boolean = false) =
            crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, numSteps, isCall, american)
        line(
            "CRR N=$numSteps: C=${fmt(treePrice(true))} P=${fmt(treePrice(false))} " +
                "P_A=${fmt(treePrice(false, true))}",
        )
    }
}

/** The full chapter 8 report. */
fun report(): String = buildString {
    line("Chapter 8: Option Pricing (illustrative numbers, book formulas)")
    oneStepSection()
    twoStepsSection()
    generalModelSection()
    americanSection()
    blackScholesSection()
}
