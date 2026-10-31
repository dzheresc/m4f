package m4f.ch03

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.pow
import kotlin.test.Test
import kotlin.test.assertFalse
import kotlin.test.assertTrue

class Ch03Test {
    private fun assertNear(expected: Double, actual: Double, eps: Double = 1e-9) =
        assertTrue(abs(expected - actual) < eps, "expected $expected, got $actual")

    @Test
    fun reportValues() {
        val tree = BinomialTree(100.0, 0.10, -0.05, 0.03)
        assertNear(112.4864, tree.expectedPrice(3, 0.6))
        assertNear(8.0 / 15, tree.riskNeutralProb())
        assertNear(0.8, TrinomialModel(0.10, 0.0, -0.10, 0.02).maxMiddleProb())
        assertNear(100 * exp(0.07), lognormalExpectedPrice(100.0, 0.05, 0.2, 1.0))
    }

    @Test
    fun returns() {
        val path = listOf(45.0, 48.0, 47.5, 50.0)
        assertNear(oneStepReturn(45.0, 50.0), compoundReturn(returns(path)))
        assertNear(logReturn(45.0, 50.0), logReturns(path).sum())
        assertNear(2.0, expectation(listOf(1.0, 3.0), listOf(0.5, 0.5)))
        assertNear(1.0, variance(listOf(1.0, 3.0), listOf(0.5, 0.5)))
    }

    @Test
    fun binomial() {
        assertNear(1.0, (0..5).sumOf { binomialProb(5, it, 0.3) })
        // Large n: C(2000, 1000) alone overflows a Double.
        assertNear(1.0, (0..2000).sumOf { binomialProb(2000, it, 0.6) })
        assertNear(0.017839011, binomialProb(2000, 1000, 0.5))
        val tree = BinomialTree(60.0, 0.2, -0.1, 0.05)
        val q = tree.riskNeutralProb()
        assertTrue(tree.isArbitrageFree())
        assertNear(0.05, tree.expectedReturn(q))
        for (n in 0 until 4) {
            assertNear(60 * 1.05.pow(n), tree.expectedPrice(n, q))
            assertNear(60 * (1 + tree.expectedReturn(0.4)).pow(n), tree.expectedPrice(n, 0.4))
            for (k in 0..n) {
                assertNear(tree.discountedPrice(n, k), tree.conditionalDiscounted(n, k))
            }
        }
        assertFalse(BinomialTree(60.0, 0.2, 0.1, 0.05).isArbitrageFree())
    }

    @Test
    fun trinomial() {
        val model = TrinomialModel(0.10, 0.0, -0.10, 0.02)
        for (pMid in listOf(0.1, 0.5, 0.79)) {
            val (pUp, _, pDown) = model.riskNeutralProbs(pMid)
            assertNear(1.0, pUp + pMid + pDown)
            assertNear(0.02, pUp * 0.10 + pDown * -0.10)
            assertTrue(pUp > 0 && pDown > 0)
        }
        assertTrue(model.riskNeutralProbs(0.81).pDown < 0)
    }

    @Test
    fun continuousLimit() {
        assertNear(0.5, normCdf(0.0), 1e-7)
        assertNear(0.9750021, normCdf(1.96), 1e-6)
        assertNear(1.0, normCdf(-1.0) + normCdf(1.0))
        assertNear(
            lognormalExpectedPrice(100.0, 0.05, 0.2, 1.0),
            binomialExpectedPrice(100.0, 0.05, 0.2, 1.0, 100000),
            1e-4,
        )
        assertNear(
            lognormalProbAbove(100.0, 0.05, 0.2, 1.0, 110.0),
            binomialProbAbove(100.0, 0.05, 0.2, 1.0, 20000, 110.0),
            1e-2,
        )
        assertNear(1.0, binomialProbAbove(100.0, 0.0, 0.2, 1.0, 7, 1e-9))
    }
}
