package m4f.ch08

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.max
import kotlin.math.min

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations.
private fun f(v: Double): String = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

private fun fl(vs: List<Double>): String = vs.joinToString(", ", "[", "]") { f(it) }

private fun StringBuilder.line(s: String) = append(s).append('\n')

private fun call(X: Double): Payoff = { s -> max(s - X, 0.0) }
private fun put(X: Double): Payoff = { s -> max(X - s, 0.0) }

fun report(): String = buildString {
    line("Chapter 8: Option Pricing (illustrative numbers, book formulas)")

    line("\n== 8.1.1 One step: S(0)=100 u=0.2 d=-0.1 r=0.1, call X=100 ==")
    val rep = oneStepReplication(100.0, 0.2, -0.1, 0.1, 20.0, 0.0)
    val q = riskNeutralProb(0.2, -0.1, 0.1)
    line("Replicate with x=${f(rep.x)} shares, y=${f(rep.y)} in the money market: C(0)=${f(rep.price)}")
    line("Risk-neutral: p*=${f(q)}, C(0) = E*(C(1))/(1+r) = ${f((q * 20.0 + (1 - q) * 0.0) / 1.1)}")

    line("\n== 8.1.2 Two steps: same tree, call X=100 ==")
    val V = optionTree(100.0, 0.2, -0.1, 0.1, 2, call(100.0))
    for (n in 0 until 3)
        line("n=$n: S=${fl((n downTo 0).map { k -> stockPrice(100.0, 0.2, -0.1, n, k) })} C=${fl(V[n].reversed())}")
    for ((n, k) in listOf(Pair(0, 0), Pair(1, 1), Pair(1, 0))) {
        val (xs, ys) = replicatingPosition(100.0, 0.2, -0.1, 0.1, V, n, k)
        line("node ($n,$k): x=${f(xs)} y=${f(ys)}")
    }

    line("\n== 8.1.3 General N-step model: N=5, X=100 ==")
    for ((name, pay) in listOf(Pair("call", call(100.0)), Pair("put", put(100.0))))
        line("$name: backward induction=${f(optionTree(100.0, 0.2, -0.1, 0.1, 5, pay)[0][0])} " +
            "sum formula=${f(europeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, pay))}")
    val spread: Payoff = { s -> min(max(s - 90.0, 0.0), 30.0) }  // bull spread 90/120
    line("bull spread 90/120: ${f(europeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, spread))}")

    line("\n== 8.1.4 Cox-Ross-Rubinstein formula ==")
    for (X in listOf(80.0, 100.0, 150.0)) {
        val c = crrCall(100.0, X, 0.2, -0.1, 0.1, 5)
        line("N=5 X=${f(X)}: m=${c.m} C=${f(c.price)} sum formula=${f(europeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, call(X)))}")
    }

    line("\n== 8.2 American options: S(0)=100 u=0.1 d=-0.1 r=0.02 N=4, put X=100 ==")
    val VA = optionTree(100.0, 0.1, -0.1, 0.02, 4, put(100.0), true)
    val VE = optionTree(100.0, 0.1, -0.1, 0.02, 4, put(100.0))
    line("P_A=${f(VA[0][0])} P_E=${f(VE[0][0])}")
    for (n in 0 until 5) line("n=$n: P_A=${fl(VA[n].reversed())}")
    line("Exercise early at (n,k): " + earlyExerciseNodes(100.0, 0.1, -0.1, 0.02, 4, put(100.0))
        .joinToString(", ") { (n, k) -> "($n,$k) S=${f(stockPrice(100.0, 0.1, -0.1, n, k))}" })

    line("\n== 8.3 Black-Scholes formula: S=100 X=100 r=0.05 sigma=0.2 T=1 ==")
    line("C=${f(bsCall(100.0, 100.0, 0.05, 0.2, 1.0))} P=${f(bsPut(100.0, 100.0, 0.05, 0.2, 1.0))} " +
        "delta=${f(bsCallDelta(100.0, 100.0, 0.05, 0.2, 1.0))}")
    for (N in listOf(10, 50, 100, 500, 1000))
        line("CRR N=$N: C=${f(crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, N, true))} " +
            "P=${f(crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, N, false))} " +
            "P_A=${f(crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, N, false, true))}")
}
