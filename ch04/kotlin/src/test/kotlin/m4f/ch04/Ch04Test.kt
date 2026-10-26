package m4f.ch04

import kotlin.math.abs
import kotlin.math.pow
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue

class Ch04Test {
    private fun near(a: Double, b: Double, eps: Double = 1e-9) = assertTrue(abs(a - b) < eps, "$a != $b")

    @Test fun selfFinancing() {
        val S = listOf(100.0, 110.0, 104.5, 114.95)
        val A = (0 until 4).map { 100.0 * 1.03.pow(it) }
        val x = listOf(5.0, 8.0, 3.0)
        val (y, V) = selfFinancing(1000.0, S, A, x)
        near(V[3], 1079.2915, 1e-4)
        for (n in 1 until 3) near(x[n] * S[n] + y[n] * A[n], V[n])
        near(gains(S, A, x, y), V[3] - V[0])
        near(discountedGains(S, A, x), V[3] / A[3] - V[0] / A[0])
    }

    @Test fun dividends() {
        val S = listOf(listOf(50.0, 55.0, 52.0, 58.0), listOf(20.0, 19.0, 21.0, 22.0))
        val A = (0 until 4).map { 1.05.pow(it) }
        val x = listOf(listOf(10.0, 12.0, 8.0), listOf(20.0, 15.0, 25.0))
        val div = listOf(listOf(0.0, 0.0, 0.0, 0.0), listOf(0.0, 0.0, 2.0, 0.0))
        near(selfFinancingMulti(1000.0, S, A, x, div).V[3] - selfFinancingMulti(1000.0, S, A, x).V[3], 2 * 15 * 1.05)
    }

    @Test fun arbitrage() {
        val rets = listOf(0.10, -0.05)
        assertTrue(isArbitrageFree(rets, 0.03))
        assertNull(arbitrageStrategy(rets, 0.03, 100.0, 100.0))
        for (r in listOf(0.10, 0.12, -0.05, -0.08)) {
            val (x, y) = arbitrageStrategy(rets, r, 100.0, 100.0)!!
            near(x * 100 + y * 100, 0.0)
            val V1 = oneStepValues(x, y, rets, r, 100.0, 100.0)
            assertTrue(V1.min() >= -1e-12 && V1.max() > 0)
        }
    }

    @Test fun ftap() {
        for (rets in listOf(listOf(0.10, -0.05), listOf(-0.10, 0.0, 0.10), listOf(0.3, -0.2, 0.05, 0.1)))
            for (r in listOf(-0.3, -0.09, 0.0, 0.04, 0.09, 0.5)) {
                val q = riskNeutralProbs(rets, r)
                assertEquals(isArbitrageFree(rets, r), q != null)
                if (q != null) {
                    near(q.sum(), 1.0)
                    near(q.indices.sumOf { q[it] * rets[it] }, r)
                    assertTrue(q.min() > 0)
                }
            }
        near(riskNeutralProbs(listOf(0.10, -0.05), 0.03)!![0], 8.0 / 15)
    }

    @Test fun martingale() {
        val q = 8.0 / 15
        val rules: List<Rule> = listOf(
            { _, m -> 1.0 + m.size - m.sum() },
            { _, m -> 3.0 * m.sum() - 2.0 },
            { n, _ -> n.toDouble() },
        )
        for (rule in rules)
            for (V0 in listOf(0.0, 100.0))
                near(expectedDiscountedValue(100.0, 0.10, -0.05, 0.03, 4, V0, rule, q), V0)
    }
}
