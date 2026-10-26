package m4f.ch06

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.pow
import kotlin.test.Test
import kotlin.test.assertTrue

class Ch06Test {
    private fun near(a: Double, b: Double, eps: Double = 1e-9) = assertTrue(abs(a - b) < eps, "$a != $b")

    @Test fun forwardPrices() {
        near(forwardPrice(100.0, 0.08, 0.75), 100 * exp(0.06))
        near(forwardPriceDividends(100.0, 0.08, 0.75, emptyList()), forwardPrice(100.0, 0.08, 0.75))
        near(forwardPriceYield(100.0, 0.08, 0.0, 0.75), forwardPrice(100.0, 0.08, 0.75))
        near(forwardPriceDividends(100.0, 0.08, 0.75, listOf(Pair(2.0, 0.25), Pair(2.0, 0.5))),
            forwardPrice(100.0, 0.08, 0.75) - 2 * exp(0.04) - 2 * exp(0.02))
        near(forwardArbitrageProfit(forwardPrice(100.0, 0.08, 0.75), 100.0, 0.08, 0.75), 0.0)
    }

    @Test fun forwardValue() {
        val F0 = forwardPrice(100.0, 0.08, 0.75)
        near(longForwardValue(F0, F0, 0.08, 0.25), 0.0)
        near(longForwardValue(F0, forwardPrice(104.0, 0.08, 0.25), 0.08, 0.25), 104 - F0 * exp(-0.08 * 0.25))
    }

    @Test fun futures() {
        val tree = futuresTree(100.0, 0.10, -0.05, 0.03, 3)
        for (n in 0 until 4)
            for (k in 0..n) near(tree[n][k], 100 * 1.1.pow(k) * 0.95.pow(n - k) * 1.03.pow(3 - n))
        val prices = listOf(106.0, 104.5, 101.0, 103.5, 99.0, 102.0)
        val days = marginAccount(prices, 100.0, 1000.0, 750.0)
        listOf(0.0, 500.0, 0.0, 0.0, 0.0).forEachIndexed { i, c -> near(days[i].call, c) }
        near(days.last().balance, 1000 + 500 + 100 * (prices.last() - prices.first()))
    }

    @Test fun hedging() {
        val dS = listOf(-6.0, -1.0, 2.0, 5.0)
        val df = listOf(-5.0, -2.0, 3.0, 4.0)
        val p = listOf(0.25, 0.25, 0.25, 0.25)
        val h = minVarianceHedgeRatio(dS, df, p)
        near(h, 29.0 / 27)
        assertTrue(hedgedStd(h, dS, df, p) < hedgedStd(h - 0.05, dS, df, p))
        assertTrue(hedgedStd(h, dS, df, p) < hedgedStd(h + 0.05, dS, df, p))
        near(hedgedStd(1.0, dS, dS, p), 0.0)
    }
}
