package m4f.ch04

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.pow

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations.
private fun f(v: Double): String = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

private fun fl(vs: List<Double>): String = vs.joinToString(", ", "[", "]") { f(it) }

private fun StringBuilder.line(s: String) = append(s).append('\n')

/** Hold one share plus one more for every down move so far. */
private fun contrarian(n: Int, moves: List<Int>): Double = 1.0 + (moves.size - moves.sum())

fun report(): String = buildString {
    line("Chapter 4: Discrete Time Market Models (illustrative numbers, book formulas)")

    line("\n== 4.1.1 Investment strategies: self-financing x(n+1)S(n) + y(n+1)A(n) = V(n) ==")
    val S = listOf(100.0, 110.0, 104.5, 114.95)
    val A = (0 until 4).map { 100.0 * 1.03.pow(it) }
    val x = listOf(5.0, 8.0, 3.0)
    val (y, V) = selfFinancing(1000.0, S, A, x)
    line("V(0)=1000, A(n)=100*1.03^n")
    for (n in 1 until 4)
        line("n=$n: S=${f(S[n])} A=${f(A[n])} x=${f(x[n - 1])} y=${f(y[n - 1])} V=${f(V[n])}")
    line("V(3)-V(0)=${f(V[3] - V[0])} gains sum=${f(gains(S, A, x, y))}")
    line("V~(3)-V~(0)=${f(V[3] / A[3] - V[0] / A[0])} discounted gains=${f(discountedGains(S, A, x))}")

    line("\n== 4.1.2 Principle of no arbitrage: S(0)=100 A(0)=100, K in {0.10, -0.05} ==")
    for (r in listOf(0.12, 0.03, -0.05)) {
        val rets = listOf(0.10, -0.05)
        if (isArbitrageFree(rets, r)) {
            line("r=${f(r)}: no arbitrage")
        } else {
            val (xs, ys) = arbitrageStrategy(rets, r, 100.0, 100.0)!!
            line("r=${f(r)}: arbitrage x=${f(xs)} y=${f(ys)} V(0)=${f(xs * 100.0 + ys * 100.0)} " +
                "V(1)=${fl(oneStepValues(xs, ys, rets, r, 100.0, 100.0))}")
        }
    }

    line("\n== 4.1.3 Binomial tree: S(0)=100 u=0.10 d=-0.05 r=0.03 N=4, x(n) = 1 + #downs so far ==")
    val q = (0.03 - -0.05) / (0.10 - -0.05)
    line("V(0)=100: E*(V~(4))=${f(expectedDiscountedValue(100.0, 0.10, -0.05, 0.03, 4, 100.0, ::contrarian, q))} " +
        "(p*=${f(q)}), E(V~(4))=${f(expectedDiscountedValue(100.0, 0.10, -0.05, 0.03, 4, 100.0, ::contrarian, 0.6))} (p=0.6)")

    line("\n== 4.1.4 Fundamental Theorem: no arbitrage <=> risk-neutral q > 0, K in {-0.10, 0, 0.10} ==")
    val rets = listOf(-0.10, 0.0, 0.10)
    for (r in listOf(-0.15, 0.02, 0.05, 0.10)) {
        val qs = riskNeutralProbs(rets, r)
        if (qs == null) {
            line("r=${f(r)}: arbitrage, no risk-neutral probability")
        } else {
            var s = 0.0
            for (i in rets.indices) s += qs[i] * rets[i]
            line("r=${f(r)}: q=${fl(qs)} sum qK=${f(s)}")
        }
    }

    line("\n== 4.2 Extended models: two stocks, dividend 2 on stock 2 at n=2, A(n)=1.05^n ==")
    val S2 = listOf(listOf(50.0, 55.0, 52.0, 58.0), listOf(20.0, 19.0, 21.0, 22.0))
    val A2 = (0 until 4).map { 1.05.pow(it) }
    val x2 = listOf(listOf(10.0, 12.0, 8.0), listOf(20.0, 15.0, 25.0))
    val div = listOf(listOf(0.0, 0.0, 0.0, 0.0), listOf(0.0, 0.0, 2.0, 0.0))
    val p2 = selfFinancingMulti(1000.0, S2, A2, x2, div)
    for (n in 1 until 4)
        line("n=$n: x1=${f(x2[0][n - 1])} x2=${f(x2[1][n - 1])} y=${f(p2.y[n - 1])} V=${f(p2.V[n])}")
    val nd = selfFinancingMulti(1000.0, S2, A2, x2)
    line("Without the dividend V(3)=${f(nd.V[3])}; it adds ${f(p2.V[3] - nd.V[3])} = 2 x 15 x 1.05")
}
