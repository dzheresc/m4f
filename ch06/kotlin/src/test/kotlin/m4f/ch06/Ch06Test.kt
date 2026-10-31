package m4f.ch06

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.pow
import kotlin.test.Test
import kotlin.test.assertTrue

class Ch06Test {
    private fun assertNear(expected: Double, actual: Double, eps: Double = 1e-9) =
        assertTrue(abs(expected - actual) < eps, "expected $expected, got $actual")

    @Test
    fun forwardPrices() {
        val plain = forwardPrice(100.0, 0.08, 0.75)
        assertNear(100 * exp(0.06), plain)
        assertNear(plain, forwardPriceDividends(100.0, 0.08, 0.75, emptyList()))
        assertNear(plain, forwardPriceYield(100.0, 0.08, 0.0, 0.75))
        assertNear(
            plain - 2 * exp(0.04) - 2 * exp(0.02),
            forwardPriceDividends(100.0, 0.08, 0.75, listOf(Pair(2.0, 0.25), Pair(2.0, 0.5))),
        )
        assertNear(0.0, forwardArbitrageProfit(plain, 100.0, 0.08, 0.75))
    }

    @Test
    fun forwardValue() {
        val f0 = forwardPrice(100.0, 0.08, 0.75)
        assertNear(0.0, longForwardValue(f0, f0, 0.08, 0.25))
        // A long forward is a long stock plus a short bond paying f0 at T.
        assertNear(
            104 - f0 * exp(-0.08 * 0.25),
            longForwardValue(f0, forwardPrice(104.0, 0.08, 0.25), 0.08, 0.25),
        )
    }

    @Test
    fun futures() {
        val tree = futuresTree(100.0, 0.10, -0.05, 0.03, 3)
        for (n in 0 until 4) {
            for (k in 0..n) {
                assertNear(100 * 1.1.pow(k) * 0.95.pow(n - k) * 1.03.pow(3 - n), tree[n][k])
            }
        }
        val prices = listOf(106.0, 104.5, 101.0, 103.5, 99.0, 102.0)
        val days = marginAccount(prices, 100.0, 1000.0, 750.0)
        listOf(0.0, 500.0, 0.0, 0.0, 0.0).forEachIndexed { i, call ->
            assertNear(call, days[i].call)
        }
        // Final balance = initial + calls + total gain.
        assertNear(1000 + 500 + 100 * (prices.last() - prices.first()), days.last().balance)
    }

    @Test
    fun hedging() {
        val ds = listOf(-6.0, -1.0, 2.0, 5.0)
        val df = listOf(-5.0, -2.0, 3.0, 4.0)
        val p = listOf(0.25, 0.25, 0.25, 0.25)
        val h = minVarianceHedgeRatio(ds, df, p)
        assertNear(29.0 / 27, h)
        assertTrue(hedgedStd(h, ds, df, p) < hedgedStd(h - 0.05, ds, df, p))
        assertTrue(hedgedStd(h, ds, df, p) < hedgedStd(h + 0.05, ds, df, p))
        assertNear(0.0, hedgedStd(1.0, ds, ds, p))
    }
}
