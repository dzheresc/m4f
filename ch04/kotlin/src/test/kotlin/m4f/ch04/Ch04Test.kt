package m4f.ch04

import kotlin.math.abs
import kotlin.math.pow
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

class Ch04Test {
    private fun assertNear(expected: Double, actual: Double, eps: Double = 1e-9) =
        assertTrue(abs(expected - actual) < eps, "expected $expected, got $actual")

    @Test
    fun selfFinancing() {
        val s = listOf(100.0, 110.0, 104.5, 114.95)
        val a = (0 until 4).map { 100.0 * 1.03.pow(it) }
        val x = listOf(5.0, 8.0, 3.0)
        val (y, values) = selfFinancing(1000.0, s, a, x)
        assertNear(1079.2915, values[3], 1e-4)
        for (n in 1 until 3) {
            assertNear(values[n], x[n] * s[n] + y[n] * a[n])
        }
        assertNear(values[3] - values[0], gains(s, a, x, y))
        assertNear(values[3] / a[3] - values[0] / a[0], discountedGains(s, a, x))
    }

    @Test
    fun dividends() {
        val s = listOf(listOf(50.0, 55.0, 52.0, 58.0), listOf(20.0, 19.0, 21.0, 22.0))
        val a = (0 until 4).map { 1.05.pow(it) }
        val x = listOf(listOf(10.0, 12.0, 8.0), listOf(20.0, 15.0, 25.0))
        val dividends = listOf(listOf(0.0, 0.0, 0.0, 0.0), listOf(0.0, 0.0, 2.0, 0.0))
        val withDividend = selfFinancingMulti(1000.0, s, a, x, dividends).values[3]
        val without = selfFinancingMulti(1000.0, s, a, x).values[3]
        assertNear(2 * 15 * 1.05, withDividend - without)
    }

    @Test
    fun arbitrage() {
        val returns = listOf(0.10, -0.05)
        assertTrue(isArbitrageFree(returns, 0.03))
        assertNull(arbitrageStrategy(returns, 0.03, 100.0, 100.0))
        for (r in listOf(0.10, 0.12, -0.05, -0.08)) {
            val (x, y) = assertNotNull(arbitrageStrategy(returns, r, 100.0, 100.0))
            assertNear(0.0, x * 100 + y * 100)
            val v1 = oneStepValues(x, y, returns, r, 100.0, 100.0)
            assertTrue(v1.min() >= -1e-12 && v1.max() > 0)
        }
    }

    @Test
    fun fundamentalTheorem() {
        val cases = listOf(
            listOf(0.10, -0.05),
            listOf(-0.10, 0.0, 0.10),
            listOf(0.3, -0.2, 0.05, 0.1),
        )
        for (returns in cases) {
            for (r in listOf(-0.3, -0.09, 0.0, 0.04, 0.09, 0.5)) {
                val q = riskNeutralProbs(returns, r)
                assertEquals(isArbitrageFree(returns, r), q != null)
                if (q != null) {
                    assertNear(1.0, q.sum())
                    assertNear(r, q.indices.sumOf { q[it] * returns[it] })
                    assertTrue(q.min() > 0)
                }
            }
        }
        assertNear(8.0 / 15, assertNotNull(riskNeutralProbs(listOf(0.10, -0.05), 0.03))[0])
    }

    @Test
    fun martingale() {
        val q = 8.0 / 15
        val rules: List<Rule> = listOf(
            { _, m -> 1.0 + m.size - m.sum() },
            { _, m -> 3.0 * m.sum() - 2.0 },
            { n, _ -> n.toDouble() },
        )
        for (rule in rules) {
            for (v0 in listOf(0.0, 100.0)) {
                assertNear(v0, expectedDiscountedValue(100.0, 0.10, -0.05, 0.03, 4, v0, rule, q))
            }
        }
    }
}
