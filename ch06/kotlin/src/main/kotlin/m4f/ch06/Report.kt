package m4f.ch06

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

/** Section 6.1.1. */
private fun StringBuilder.forwardPriceSection(f0: Double) {
    line("\n== 6.1.1 Forward price: F(0,T) = S(0) e^(rT), S(0)=100 r=0.08 T=0.75 ==")
    line("No dividends: F(0,T)=${fmt(f0)}")
    val withDividends =
        forwardPriceDividends(100.0, 0.08, 0.75, listOf(Pair(2.0, 0.25), Pair(2.0, 0.5)))
    line("Dividends 2 at t=0.25 and 2 at t=0.5: F(0,T)=${fmt(withDividends)}")
    line("Dividend yield 3%: F(0,T)=${fmt(forwardPriceYield(100.0, 0.08, 0.03, 0.75))}")
    line(
        "Currency 1.25 USD per EUR, r_USD=0.05 r_EUR=0.03, T=1: " +
            "F=${fmt(forwardPriceYield(1.25, 0.05, 0.03, 1.0))}",
    )
    for (quoted in listOf(108.0, 104.0)) {
        val side = if (quoted > f0) {
            "cash and carry: buy stock, sell forward"
        } else {
            "reverse: short stock, buy forward"
        }
        val profit = forwardArbitrageProfit(quoted, 100.0, 0.08, 0.75)
        line("Quoted F=${fmt(quoted)}: $side, profit at T=${fmt(profit)}")
    }
}

/** Section 6.1.2. */
private fun StringBuilder.forwardValueSection(f0: Double) {
    line("\n== 6.1.2 Value of a forward contract: V(t) = (F(t,T) - F(0,T)) e^(-r(T-t)) ==")
    for (st in listOf(104.0, 98.0)) {
        val ft = forwardPrice(st, 0.08, 0.25)
        line(
            "t=0.5 S(t)=${fmt(st)}: F(t,T)=${fmt(ft)} " +
                "long V(t)=${fmt(longForwardValue(f0, ft, 0.08, 0.25))} " +
                "short V(t)=${fmt(-longForwardValue(f0, ft, 0.08, 0.25))}",
        )
    }
}

/** Section 6.2.1. */
private fun StringBuilder.futuresPricingSection() {
    line(
        "\n== 6.2.1 Futures pricing: f(n) = E*(S(N) | S(n)), S(0)=100 u=0.10 d=-0.05 r=0.03 " +
            "N=3 ==",
    )
    val tree = futuresTree(100.0, 0.10, -0.05, 0.03, 3)
    for (n in 0 until 4) {
        val forward = (n downTo 0).map { k ->
            100.0 * 1.1.pow(k) * 0.95.pow(n - k) * 1.03.pow(3 - n)
        }
        line("n=$n: futures=${fmtList(tree[n].reversed())} forward=${fmtList(forward)}")
    }
    line("Marking to market: 100 units, initial margin 1000, maintenance 750")
    val prices = listOf(106.0, 104.5, 101.0, 103.5, 99.0, 102.0)
    marginAccount(prices, 100.0, 1000.0, 750.0).forEachIndexed { i, day ->
        line(
            "day ${i + 1}: f=${fmt(prices[i + 1])} balance=${fmt(day.balance)} " +
                "margin call=${fmt(day.call)}",
        )
    }
}

/** Section 6.2.2. */
private fun StringBuilder.hedgingSection() {
    line("\n== 6.2.2 Hedging with futures: four equally likely scenarios ==")
    val ds = listOf(-6.0, -1.0, 2.0, 5.0)
    val df = listOf(-5.0, -2.0, 3.0, 4.0)
    val p = listOf(0.25, 0.25, 0.25, 0.25)
    val h = minVarianceHedgeRatio(ds, df, p)
    line("dS=${fmtList(ds)} df=${fmtList(df)}")
    line(
        "h*=${fmt(h)}; std of spot change: unhedged ${fmt(hedgedStd(0.0, ds, df, p))}, " +
            "h=1 ${fmt(hedgedStd(1.0, ds, df, p))}, h* ${fmt(hedgedStd(h, ds, df, p))}",
    )
    val cost = listOf(112.0 - (112.0 - 106.0), 95.0 - (95.0 - 106.0))
    line(
        "Perfect hedge: buy at T with a long futures at f(0)=106; spot at T 112 or 95 -> " +
            "cost ${fmtList(cost)}",
    )
}

/** The full chapter 6 report. */
fun report(): String = buildString {
    line("Chapter 6: Forward and Futures Contracts (illustrative numbers, book formulas)")
    val f0 = forwardPrice(100.0, 0.08, 0.75)
    forwardPriceSection(f0)
    forwardValueSection(f0)
    futuresPricingSection()
    hedgingSection()
}
