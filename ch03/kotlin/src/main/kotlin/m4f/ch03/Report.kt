package m4f.ch03

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.pow
import kotlin.math.sqrt

// Round-half-even on the exact binary value, like C++ std::format and Python, so the report is
// byte-identical across the three implementations. Adding 0.0 turns -0.0 into 0.0.
private fun fmt(v: Double): String =
    BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

private fun StringBuilder.line(s: String) {
    append(s).append('\n')
}

/** Section 3.1.1. */
private fun StringBuilder.returnsSection() {
    line("\n== 3.1.1 Return: K(n) = (S(n) - S(n-1)) / S(n-1), k(n) = ln(S(n) / S(n-1)) ==")
    val path = listOf(45.0, 48.0, 47.5, 50.0)
    val oneStep = returns(path)
    val logs = logReturns(path)
    for (n in 1 until path.size) {
        line("n=$n: S=${fmt(path[n])} K=${fmt(oneStep[n - 1])} k=${fmt(logs[n - 1])}")
    }
    line(
        "K(0,3) from S: ${fmt(oneStepReturn(path[0], path[3]))}, " +
            "from (1+K(1))(1+K(2))(1+K(3))-1: ${fmt(compoundReturn(oneStep))}",
    )
    line(
        "k(0,3) from S: ${fmt(logReturn(path[0], path[3]))}, " +
            "k(1)+k(2)+k(3): ${fmt(logs[0] + logs[1] + logs[2])}",
    )
    line("S(0)=45, S(1)=48 with dividend 2: K(1)=${fmt(oneStepReturn(45.0, 48.0, 2.0))}")
}

/** Section 3.1.2. */
private fun StringBuilder.expectedReturnSection() {
    line("\n== 3.1.2 Expected return ==")
    val prices = listOf(52.0, 48.0, 40.0)
    val probs = listOf(0.25, 0.5, 0.25)
    val oneStep = prices.map { oneStepReturn(45.0, it) }
    val mean = expectation(oneStep, probs)
    line(
        "S(0)=45, S(1) in {52, 48, 40} with p={0.25, 0.5, 0.25}: " +
            "E(K)=${fmt(mean)} sd(K)=${fmt(sqrt(variance(oneStep, probs)))}",
    )
    line(
        "E(S(1)) = (1 + E(K)) S(0) = ${fmt((1 + mean) * 45.0)}, " +
            "directly ${fmt(expectation(prices, probs))}",
    )
    line(
        "Independent steps, E(K(1))=0.10, E(K(2))=0.05: " +
            "E(K(0,2)) = ${fmt(compoundReturn(listOf(0.10, 0.05)))}",
    )
}

/** Sections 3.2, 3.2.1 and 3.2.2. */
private fun StringBuilder.binomialSection(tree: BinomialTree) {
    line("\n== 3.2 Binomial tree model: S(0)=100 u=0.10 d=-0.05 r=0.03 p=0.6 ==")
    for (n in 0 until 4) {
        val nodes = (n downTo 0).joinToString(" ") { k ->
            "${fmt(tree.price(n, k))}(${fmt(binomialProb(n, k, 0.6))})"
        }
        line("n=$n: $nodes")
    }
    line(
        "E(K(n))=${fmt(tree.expectedReturn(0.6))} E(S(3))=${fmt(tree.expectedPrice(3, 0.6))} " +
            "(1+E(K))^3 S(0)=${fmt((1 + tree.expectedReturn(0.6)).pow(3) * 100.0)}",
    )

    line("\n== 3.2.1 Risk-neutral probability: p* = (r - d) / (u - d) ==")
    val q = tree.riskNeutralProb()
    val free = if (tree.isArbitrageFree()) "yes" else "no"
    line("d < r < u: $free, p*=${fmt(q)}, E*(K)=${fmt(tree.expectedReturn(q))}")
    line(
        "E*(S(1))=${fmt(tree.expectedPrice(1, q))} (1+r)S(0)=${fmt(1.03 * 100.0)}; " +
            "E*(S(3))=${fmt(tree.expectedPrice(3, q))} (1+r)^3 S(0)=${fmt(1.03.pow(3) * 100.0)}",
    )

    line("\n== 3.2.2 Martingale property: E*(S~(n+1) | S(n)) = S~(n) ==")
    var worst = 0.0
    for (n in 0 until 3) {
        for (k in 0..n) {
            worst = max(worst, abs(tree.conditionalDiscounted(n, k) - tree.discountedPrice(n, k)))
        }
    }
    for (k in 1 downTo 0) {
        line(
            "node (1,$k): S~(1)=${fmt(tree.discountedPrice(1, k))} " +
                "E*(S~(2)|.)=${fmt(tree.conditionalDiscounted(1, k))}",
        )
    }
    line("Largest deviation over all nodes n<3: ${fmt(worst)}")
}

/** Section 3.3.1. */
private fun StringBuilder.trinomialSection() {
    line("\n== 3.3.1 Trinomial tree: u=0.10 m=0 d=-0.10 r=0.02, S(0)=100 ==")
    val model = TrinomialModel(0.10, 0.0, -0.10, 0.02)
    line("Risk-neutral probabilities exist for 0 < p_m < ${fmt(model.maxMiddleProb())}")
    for (pMid in listOf(0.2, 0.4, 0.6)) {
        val (pUp, _, pDown) = model.riskNeutralProbs(pMid)
        val call = pUp * 10.0 / 1.02
        val expected = pUp * 0.10 + pMid * 0.0 + pDown * -0.10
        line(
            "p_m=${fmt(pMid)}: p_u=${fmt(pUp)} p_d=${fmt(pDown)} " +
                "E*(K)=${fmt(expected)} call(K=100) price=${fmt(call)}",
        )
    }
}

/** Section 3.3.2. */
private fun StringBuilder.continuousLimitSection() {
    line("\n== 3.3.2 Continuous-time limit: S(0)=100 m=0.05 sigma=0.2 T=1 ==")
    for (numSteps in listOf(1, 10, 100, 1000, 10000)) {
        val mean = binomialExpectedPrice(100.0, 0.05, 0.2, 1.0, numSteps)
        val prob = binomialProbAbove(100.0, 0.05, 0.2, 1.0, numSteps, 110.0)
        line("N=$numSteps: E(S(T))=${fmt(mean)} P(S(T)>110)=${fmt(prob)}")
    }
    val mean = lognormalExpectedPrice(100.0, 0.05, 0.2, 1.0)
    val prob = lognormalProbAbove(100.0, 0.05, 0.2, 1.0, 110.0)
    line("Limit: E(S(T))=${fmt(mean)} P(S(T)>110)=${fmt(prob)}")
}

/** The chapter's worked examples as text. */
fun report(): String = buildString {
    line("Chapter 3: Risky Assets (illustrative numbers, book formulas)")
    returnsSection()
    expectedReturnSection()
    binomialSection(BinomialTree(100.0, 0.10, -0.05, 0.03))
    trinomialSection()
    continuousLimitSection()
}
