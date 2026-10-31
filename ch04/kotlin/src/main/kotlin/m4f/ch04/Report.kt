package m4f.ch04

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.pow

// Round-half-even on the exact binary value, like C++ std::format and Python, so the report is
// byte-identical across the three implementations. Adding 0.0 turns -0.0 into 0.0.
private fun fmt(v: Double): String =
    BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

/** "[a, b, c]" with each entry formatted by [fmt]. */
private fun fmtList(values: List<Double>): String = values.joinToString(", ", "[", "]") { fmt(it) }

private fun StringBuilder.line(s: String) {
    append(s).append('\n')
}

/** Hold one share plus one more for every down move so far. */
@Suppress("UNUSED_PARAMETER")
private fun contrarian(n: Int, moves: List<Int>): Double = 1.0 + (moves.size - moves.sum())

/** Section 4.1.1. */
private fun StringBuilder.strategiesSection() {
    line("\n== 4.1.1 Investment strategies: self-financing x(n+1)S(n) + y(n+1)A(n) = V(n) ==")
    val s = listOf(100.0, 110.0, 104.5, 114.95)
    val a = (0 until 4).map { 100.0 * 1.03.pow(it) }
    val x = listOf(5.0, 8.0, 3.0)
    val (y, values) = selfFinancing(1000.0, s, a, x)
    line("V(0)=1000, A(n)=100*1.03^n")
    for (n in 1 until 4) {
        line(
            "n=$n: S=${fmt(s[n])} A=${fmt(a[n])} x=${fmt(x[n - 1])} y=${fmt(y[n - 1])} " +
                "V=${fmt(values[n])}",
        )
    }
    line("V(3)-V(0)=${fmt(values[3] - values[0])} gains sum=${fmt(gains(s, a, x, y))}")
    line(
        "V~(3)-V~(0)=${fmt(values[3] / a[3] - values[0] / a[0])} " +
            "discounted gains=${fmt(discountedGains(s, a, x))}",
    )
}

/** Section 4.1.2. */
private fun StringBuilder.noArbitrageSection() {
    line("\n== 4.1.2 Principle of no arbitrage: S(0)=100 A(0)=100, K in {0.10, -0.05} ==")
    for (r in listOf(0.12, 0.03, -0.05)) {
        val returns = listOf(0.10, -0.05)
        if (isArbitrageFree(returns, r)) {
            line("r=${fmt(r)}: no arbitrage")
        } else {
            val (x, y) = checkNotNull(arbitrageStrategy(returns, r, 100.0, 100.0))
            val values = oneStepValues(x, y, returns, r, 100.0, 100.0)
            line(
                "r=${fmt(r)}: arbitrage x=${fmt(x)} y=${fmt(y)} " +
                    "V(0)=${fmt(x * 100.0 + y * 100.0)} V(1)=${fmtList(values)}",
            )
        }
    }
}

/** Section 4.1.3. */
private fun StringBuilder.binomialSection() {
    line(
        "\n== 4.1.3 Binomial tree: S(0)=100 u=0.10 d=-0.05 r=0.03 N=4, " +
            "x(n) = 1 + #downs so far ==",
    )
    val q = (0.03 - -0.05) / (0.10 - -0.05)
    val riskNeutral = expectedDiscountedValue(100.0, 0.10, -0.05, 0.03, 4, 100.0, ::contrarian, q)
    val real = expectedDiscountedValue(100.0, 0.10, -0.05, 0.03, 4, 100.0, ::contrarian, 0.6)
    line("V(0)=100: E*(V~(4))=${fmt(riskNeutral)} (p*=${fmt(q)}), E(V~(4))=${fmt(real)} (p=0.6)")
}

/** Section 4.1.4. */
private fun StringBuilder.fundamentalTheoremSection() {
    line(
        "\n== 4.1.4 Fundamental Theorem: no arbitrage <=> risk-neutral q > 0, " +
            "K in {-0.10, 0, 0.10} ==",
    )
    val returns = listOf(-0.10, 0.0, 0.10)
    for (r in listOf(-0.15, 0.02, 0.05, 0.10)) {
        val q = riskNeutralProbs(returns, r)
        if (q == null) {
            line("r=${fmt(r)}: arbitrage, no risk-neutral probability")
        } else {
            var sum = 0.0
            for (i in returns.indices) {
                sum += q[i] * returns[i]
            }
            line("r=${fmt(r)}: q=${fmtList(q)} sum qK=${fmt(sum)}")
        }
    }
}

/** Section 4.2. */
private fun StringBuilder.extendedModelsSection() {
    line("\n== 4.2 Extended models: two stocks, dividend 2 on stock 2 at n=2, A(n)=1.05^n ==")
    val s = listOf(listOf(50.0, 55.0, 52.0, 58.0), listOf(20.0, 19.0, 21.0, 22.0))
    val a = (0 until 4).map { 1.05.pow(it) }
    val x = listOf(listOf(10.0, 12.0, 8.0), listOf(20.0, 15.0, 25.0))
    val dividends = listOf(listOf(0.0, 0.0, 0.0, 0.0), listOf(0.0, 0.0, 2.0, 0.0))
    val withDividend = selfFinancingMulti(1000.0, s, a, x, dividends)
    for (n in 1 until 4) {
        line(
            "n=$n: x1=${fmt(x[0][n - 1])} x2=${fmt(x[1][n - 1])} " +
                "y=${fmt(withDividend.y[n - 1])} V=${fmt(withDividend.values[n])}",
        )
    }
    val without = selfFinancingMulti(1000.0, s, a, x)
    line(
        "Without the dividend V(3)=${fmt(without.values[3])}; " +
            "it adds ${fmt(withDividend.values[3] - without.values[3])} = 2 x 15 x 1.05",
    )
}

/** The chapter's worked examples as text. */
fun report(): String = buildString {
    line("Chapter 4: Discrete Time Market Models (illustrative numbers, book formulas)")
    strategiesSection()
    noArbitrageSection()
    binomialSection()
    fundamentalTheoremSection()
    extendedModelsSection()
}
