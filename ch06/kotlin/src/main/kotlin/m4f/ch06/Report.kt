package m4f.ch06

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.pow

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations.
private fun f(v: Double): String = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

private fun fl(vs: List<Double>): String = vs.joinToString(", ", "[", "]") { f(it) }

private fun StringBuilder.line(s: String) = append(s).append('\n')

fun report(): String = buildString {
    line("Chapter 6: Forward and Futures Contracts (illustrative numbers, book formulas)")

    line("\n== 6.1.1 Forward price: F(0,T) = S(0) e^(rT), S(0)=100 r=0.08 T=0.75 ==")
    val F0 = forwardPrice(100.0, 0.08, 0.75)
    line("No dividends: F(0,T)=${f(F0)}")
    line("Dividends 2 at t=0.25 and 2 at t=0.5: F(0,T)=" +
        f(forwardPriceDividends(100.0, 0.08, 0.75, listOf(Pair(2.0, 0.25), Pair(2.0, 0.5)))))
    line("Dividend yield 3%: F(0,T)=${f(forwardPriceYield(100.0, 0.08, 0.03, 0.75))}")
    line("Currency 1.25 USD per EUR, r_USD=0.05 r_EUR=0.03, T=1: F=${f(forwardPriceYield(1.25, 0.05, 0.03, 1.0))}")
    for (Fq in listOf(108.0, 104.0)) {
        val side = if (Fq > F0) "cash and carry: buy stock, sell forward" else "reverse: short stock, buy forward"
        line("Quoted F=${f(Fq)}: $side, profit at T=${f(forwardArbitrageProfit(Fq, 100.0, 0.08, 0.75))}")
    }

    line("\n== 6.1.2 Value of a forward contract: V(t) = (F(t,T) - F(0,T)) e^(-r(T-t)) ==")
    for (St in listOf(104.0, 98.0)) {
        val Ft = forwardPrice(St, 0.08, 0.25)
        line("t=0.5 S(t)=${f(St)}: F(t,T)=${f(Ft)} long V(t)=${f(longForwardValue(F0, Ft, 0.08, 0.25))} " +
            "short V(t)=${f(-longForwardValue(F0, Ft, 0.08, 0.25))}")
    }

    line("\n== 6.2.1 Futures pricing: f(n) = E*(S(N) | S(n)), S(0)=100 u=0.10 d=-0.05 r=0.03 N=3 ==")
    val tree = futuresTree(100.0, 0.10, -0.05, 0.03, 3)
    for (n in 0 until 4) {
        val fwd = (n downTo 0).map { k -> 100.0 * 1.1.pow(k) * 0.95.pow(n - k) * 1.03.pow(3 - n) }
        line("n=$n: futures=${fl(tree[n].reversed())} forward=${fl(fwd)}")
    }
    line("Marking to market: 100 units, initial margin 1000, maintenance 750")
    val prices = listOf(106.0, 104.5, 101.0, 103.5, 99.0, 102.0)
    marginAccount(prices, 100.0, 1000.0, 750.0).forEachIndexed { i, day ->
        line("day ${i + 1}: f=${f(prices[i + 1])} balance=${f(day.balance)} margin call=${f(day.call)}")
    }

    line("\n== 6.2.2 Hedging with futures: four equally likely scenarios ==")
    val dS = listOf(-6.0, -1.0, 2.0, 5.0)
    val df = listOf(-5.0, -2.0, 3.0, 4.0)
    val p = listOf(0.25, 0.25, 0.25, 0.25)
    val h = minVarianceHedgeRatio(dS, df, p)
    line("dS=${fl(dS)} df=${fl(df)}")
    line("h*=${f(h)}; std of spot change: unhedged ${f(hedgedStd(0.0, dS, df, p))}, " +
        "h=1 ${f(hedgedStd(1.0, dS, df, p))}, h* ${f(hedgedStd(h, dS, df, p))}")
    line("Perfect hedge: buy at T with a long futures at f(0)=106; spot at T 112 or 95 -> " +
        "cost ${fl(listOf(112.0 - (112.0 - 106.0), 95.0 - (95.0 - 106.0)))}")
}
