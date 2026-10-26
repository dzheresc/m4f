package m4f.ch03

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.pow
import kotlin.test.Test
import kotlin.test.assertFalse
import kotlin.test.assertTrue

class Ch03Test {
    private fun near(a: Double, b: Double, eps: Double = 1e-9) = assertTrue(abs(a - b) < eps, "$a != $b")

    @Test fun reportValues() {
        near(BinomialTree(100.0, 0.10, -0.05, 0.03).expectedPrice(3, 0.6), 112.4864)
        near(BinomialTree(100.0, 0.10, -0.05, 0.03).riskNeutralProb(), 8.0 / 15)
        near(TrinomialModel(0.10, 0.0, -0.10, 0.02).maxMiddleProb(), 0.8)
        near(lognormalExpectedPrice(100.0, 0.05, 0.2, 1.0), 100 * exp(0.07))
    }

    @Test fun returns() {
        val path = listOf(45.0, 48.0, 47.5, 50.0)
        near(compoundReturn(returns(path)), oneStepReturn(45.0, 50.0))
        near(logReturns(path).sum(), logReturn(45.0, 50.0))
        near(expectation(listOf(1.0, 3.0), listOf(0.5, 0.5)), 2.0)
        near(variance(listOf(1.0, 3.0), listOf(0.5, 0.5)), 1.0)
    }

    @Test fun binomial() {
        near((0..5).sumOf { binomialProb(5, it, 0.3) }, 1.0)
        val tree = BinomialTree(60.0, 0.2, -0.1, 0.05)
        val q = tree.riskNeutralProb()
        assertTrue(tree.isArbitrageFree())
        near(tree.expectedReturn(q), 0.05)
        for (n in 0 until 4) {
            near(tree.expectedPrice(n, q), 60 * 1.05.pow(n))
            near(tree.expectedPrice(n, 0.4), 60 * (1 + tree.expectedReturn(0.4)).pow(n))
            for (k in 0..n) near(tree.conditionalDiscounted(n, k), tree.discountedPrice(n, k))
        }
        assertFalse(BinomialTree(60.0, 0.2, 0.1, 0.05).isArbitrageFree())
    }

    @Test fun trinomial() {
        val tri = TrinomialModel(0.10, 0.0, -0.10, 0.02)
        for (pm in listOf(0.1, 0.5, 0.79)) {
            val (pu, _, pd) = tri.riskNeutralProbs(pm)
            near(pu + pm + pd, 1.0)
            near(pu * 0.10 + pd * -0.10, 0.02)
            assertTrue(pu > 0 && pd > 0)
        }
        assertTrue(tri.riskNeutralProbs(0.81).pd < 0)
    }

    @Test fun continuousLimit() {
        near(normCdf(0.0), 0.5, 1e-7)
        near(normCdf(1.96), 0.9750021, 1e-6)
        near(normCdf(-1.0) + normCdf(1.0), 1.0)
        near(binomialExpectedPrice(100.0, 0.05, 0.2, 1.0, 100000), lognormalExpectedPrice(100.0, 0.05, 0.2, 1.0), 1e-4)
        near(binomialProbAbove(100.0, 0.05, 0.2, 1.0, 20000, 110.0), lognormalProbAbove(100.0, 0.05, 0.2, 1.0, 110.0), 1e-2)
        near(binomialProbAbove(100.0, 0.0, 0.2, 1.0, 7, 1e-9), 1.0)
    }
}
