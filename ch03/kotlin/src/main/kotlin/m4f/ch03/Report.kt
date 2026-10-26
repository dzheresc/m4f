package m4f.ch03

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.pow
import kotlin.math.sqrt

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations.
private fun f(v: Double): String = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

private fun StringBuilder.line(s: String) = append(s).append('\n')

fun report(): String = buildString {
    line("Chapter 3: Risky Assets (illustrative numbers, book formulas)")

    line("\n== 3.1.1 Return: K(n) = (S(n) - S(n-1)) / S(n-1), k(n) = ln(S(n) / S(n-1)) ==")
    val path = listOf(45.0, 48.0, 47.5, 50.0)
    val Ks = returns(path)
    val ks = logReturns(path)
    for (n in 1 until path.size) line("n=$n: S=${f(path[n])} K=${f(Ks[n - 1])} k=${f(ks[n - 1])}")
    line("K(0,3) from S: ${f(oneStepReturn(path[0], path[3]))}, " +
        "from (1+K(1))(1+K(2))(1+K(3))-1: ${f(compoundReturn(Ks))}")
    line("k(0,3) from S: ${f(logReturn(path[0], path[3]))}, k(1)+k(2)+k(3): ${f(ks[0] + ks[1] + ks[2])}")
    line("S(0)=45, S(1)=48 with dividend 2: K(1)=${f(oneStepReturn(45.0, 48.0, 2.0))}")

    line("\n== 3.1.2 Expected return ==")
    val S1s = listOf(52.0, 48.0, 40.0)
    val probs = listOf(0.25, 0.5, 0.25)
    val K1s = S1s.map { oneStepReturn(45.0, it) }
    val EK = expectation(K1s, probs)
    line("S(0)=45, S(1) in {52, 48, 40} with p={0.25, 0.5, 0.25}: " +
        "E(K)=${f(EK)} sd(K)=${f(sqrt(variance(K1s, probs)))}")
    line("E(S(1)) = (1 + E(K)) S(0) = ${f((1 + EK) * 45.0)}, directly ${f(expectation(S1s, probs))}")
    line("Independent steps, E(K(1))=0.10, E(K(2))=0.05: E(K(0,2)) = ${f(compoundReturn(listOf(0.10, 0.05)))}")

    line("\n== 3.2 Binomial tree model: S(0)=100 u=0.10 d=-0.05 r=0.03 p=0.6 ==")
    val tree = BinomialTree(100.0, 0.10, -0.05, 0.03)
    for (n in 0 until 4) {
        val nodes = (n downTo 0).joinToString(" ") { k -> "${f(tree.price(n, k))}(${f(binomialProb(n, k, 0.6))})" }
        line("n=$n: $nodes")
    }
    line("E(K(n))=${f(tree.expectedReturn(0.6))} E(S(3))=${f(tree.expectedPrice(3, 0.6))} " +
        "(1+E(K))^3 S(0)=${f((1 + tree.expectedReturn(0.6)).pow(3) * 100.0)}")

    line("\n== 3.2.1 Risk-neutral probability: p* = (r - d) / (u - d) ==")
    val q = tree.riskNeutralProb()
    line("d < r < u: ${if (tree.isArbitrageFree()) "yes" else "no"}, p*=${f(q)}, E*(K)=${f(tree.expectedReturn(q))}")
    line("E*(S(1))=${f(tree.expectedPrice(1, q))} (1+r)S(0)=${f(1.03 * 100.0)}; " +
        "E*(S(3))=${f(tree.expectedPrice(3, q))} (1+r)^3 S(0)=${f(1.03.pow(3) * 100.0)}")

    line("\n== 3.2.2 Martingale property: E*(S~(n+1) | S(n)) = S~(n) ==")
    var worst = 0.0
    for (n in 0 until 3)
        for (k in 0..n)
            worst = max(worst, abs(tree.conditionalDiscounted(n, k) - tree.discountedPrice(n, k)))
    for (k in 1 downTo 0)
        line("node (1,$k): S~(1)=${f(tree.discountedPrice(1, k))} E*(S~(2)|.)=${f(tree.conditionalDiscounted(1, k))}")
    line("Largest deviation over all nodes n<3: ${f(worst)}")

    line("\n== 3.3.1 Trinomial tree: u=0.10 m=0 d=-0.10 r=0.02, S(0)=100 ==")
    val tri = TrinomialModel(0.10, 0.0, -0.10, 0.02)
    line("Risk-neutral probabilities exist for 0 < p_m < ${f(tri.maxMiddleProb())}")
    for (pm in listOf(0.2, 0.4, 0.6)) {
        val (pu, _, pd) = tri.riskNeutralProbs(pm)
        val call = pu * 10.0 / 1.02
        line("p_m=${f(pm)}: p_u=${f(pu)} p_d=${f(pd)} " +
            "E*(K)=${f(pu * 0.10 + pm * 0.0 + pd * -0.10)} call(K=100) price=${f(call)}")
    }

    line("\n== 3.3.2 Continuous-time limit: S(0)=100 m=0.05 sigma=0.2 T=1 ==")
    for (N in listOf(1, 10, 100, 1000, 10000))
        line("N=$N: E(S(T))=${f(binomialExpectedPrice(100.0, 0.05, 0.2, 1.0, N))} " +
            "P(S(T)>110)=${f(binomialProbAbove(100.0, 0.05, 0.2, 1.0, N, 110.0))}")
    line("Limit: E(S(T))=${f(lognormalExpectedPrice(100.0, 0.05, 0.2, 1.0))} " +
        "P(S(T)>110)=${f(lognormalProbAbove(100.0, 0.05, 0.2, 1.0, 110.0))}")
}
