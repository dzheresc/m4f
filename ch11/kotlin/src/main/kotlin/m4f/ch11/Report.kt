package m4f.ch11

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.exp
import kotlin.math.max

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations. A tiny
// negative that rounds to zero prints as "-0.0000" there, so it does here too.
private fun f(v: Double): String {
    val s = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()
    return if (v < 0 && !s.startsWith("-")) "-$s" else s
}

private fun fl(vs: List<Double>): String = vs.joinToString(", ", "[", "]") { f(it) }

private fun StringBuilder.line(s: String) = append(s).append('\n')

fun report(): String = buildString {
    line("Chapter 11: Stochastic Interest Rates (illustrative numbers, book formulas)")

    val N = 4
    val tau = 1.0
    val rates = rateLattice(0.05, 0.01, -0.01, N)
    val probs = constantProbs(0.5, N)

    line("\n== 11.1 Binomial tree model: r(n,k) = 0.05 + 0.01 k - 0.01 (n-k), tau=1, N=4 ==")
    for (n in 0..N)
        line("n=$n: r=${fl(rates[n].reversed())} B(n,n+1)=${fl(rates[n].reversed().map { oneStepPrice(it, tau) })}")
    for (path in listOf("uuuu", "udud", "dddd"))
        line("Money market account along $path: A=${fl(moneyMarket(rates, tau, path))}")

    line("\n== 11.2 Arbitrage pricing of bonds with p*=0.5 ==")
    val B0 = bondPrices(rates, probs, tau, N)
    line("B(0,N) for N=1..4: " + fl(B0))
    line("Term structure y(0,N): " + fl(B0.withIndex().map { (m, B) -> yieldRate(B, 0, m + 1, tau) }))
    val B4 = bondLattice(rates, probs, tau, N)
    for (n in 0..N) line("B(n,4) at n=$n: ${fl(B4[n].reversed())}")
    val B3 = bondLattice(rates, probs, tau, 3)
    val B2 = bondLattice(rates, probs, tau, 2)
    val lattices = listOf(2 to B2, 3 to B3, 4 to B4)
    line("Term structure after one step: up node y(1,N)=" + fl(lattices.map { (m, B) -> yieldRate(B[1][1], 1, m, tau) }) +
        " down node y(1,N)=" + fl(lattices.map { (m, B) -> yieldRate(B[1][0], 1, m, tau) }))
    line("Risk-neutral one-step return E*[B(1,N)]/B(0,N) for N=2,3,4: " +
        fl(lattices.map { (_, B) -> (0.5 * B[1][1] + 0.5 * B[1][0]) / B[0][0] }) + " = e^(r(0) tau) = ${f(exp(0.05 * tau))}")

    line("\n== 11.2.1 Risk-neutral probabilities implied by bond prices ==")
    for ((m, B) in listOf(4 to B4, 3 to B3, 2 to B2)) {
        val P = impliedProbs(B, rates, tau)
        line("from B(n,$m): " + P.indices.joinToString(" ") { n -> "n=$n:${fl(P[n].reversed())}" })
    }
    val probs6 = constantProbs(0.6, N)
    val B46 = bondLattice(rates, probs6, tau, N)
    line("Bonds priced with p*=0.6: B(0,4)=${f(B46[0][0])} (vs ${f(B4[0][0])}); implied p* at (0,0): ${f(impliedProb(B46, rates, tau, 0, 0))}")
    val wrong = B2.map { it.toMutableList() }
    wrong[0][0] = B2[0][0] - 0.01
    line("A two-year bond quoted at ${f(wrong[0][0])} instead of ${f(B2[0][0])} implies p*=${f(impliedProb(wrong, rates, tau, 0, 0))} " +
        "at (0,0), inconsistent with the other bonds: arbitrage")

    line("\n== 11.3.1 Options on the 4-year bond, expiry n=2, p*=0.5 ==")
    val X = 0.90
    for ((name, isCall) in listOf("call" to true, "put" to false)) {
        val V = bondOption(rates, probs, tau, N, 2, X, isCall)
        line("European $name X=${f(X)}: payoff at n=2 ${fl(V[2].reversed())}, price ${f(V[0][0])}")
    }
    val VA = bondOption(rates, probs, tau, N, 2, X, false, american = true)
    line("American put X=${f(X)}: price ${f(VA[0][0])} = immediate exercise X - B(0,4) = ${f(X - B4[0][0])}; " +
        "values at n=1 ${fl(VA[1].reversed())} = exercise values ${fl(B4[1].reversed().map { max(X - it, 0.0) })}")
    line("American call X=0.85 on the 4-year bond, expiry n=3: " +
        "${f(bondOption(rates, probs, tau, N, 3, 0.85, true, american = true)[0][0])} vs European " +
        "${f(bondOption(rates, probs, tau, N, 3, 0.85, true)[0][0])}")
    line("Call on the 2-year bond expiring at n=1, X=0.95: " + f(bondOption(rates, probs, tau, 2, 1, 0.95, true)[0][0]))

    line("\n== 11.3.2 Swaps: fixed K against floating L(n,k) = e^(r tau) - 1, 4 annual payments ==")
    for (n in 0 until N) line("n=$n: L=${fl((n downTo 0).map { k -> floatingRate(rates, tau, n, k) })}")
    val K = swapRate(B0)
    line("Swap rate K = (1 - B(0,4)) / sum B(0,n) = ${f(K)}")
    val S = payerSwap(rates, probs, tau, N, K)
    line("Payer swap value at K: ${f(S[0][0])}; at n=1: ${fl(S[1].reversed())}")
    for (Kx in listOf(0.04, 0.05, 0.06)) line("Payer swap at K=${f(Kx)}: ${f(payerSwap(rates, probs, tau, N, Kx)[0][0])}")

    line("\n== 11.3.3 Caps and floors on the same floating rate ==")
    for (Kx in listOf(0.04, K, 0.06)) {
        val c = cap(rates, probs, tau, N, Kx)[0][0]
        val fo = floor(rates, probs, tau, N, Kx)[0][0]
        val s = payerSwap(rates, probs, tau, N, Kx)[0][0]
        line("K=${f(Kx)}: cap=${f(c)} floor=${f(fo)} cap-floor=${f(c - fo)} = payer swap ${f(s)}")
    }
    val cs = (0 until N).map { caplet(rates, probs, tau, it, 0.05) }
    line("Caplets at K=0.05 by fixing date n=0..3: " + fl(cs) + ", sum ${f(plainSum(cs))} = cap ${f(cap(rates, probs, tau, N, 0.05)[0][0])}")
}
