package m4f.ch05

import kotlin.math.abs
import kotlin.test.Test
import kotlin.test.assertTrue

class Ch05Test {
    private val means = listOf(0.10, 0.15, 0.20)
    private val cov = covarianceMatrix(
        listOf(0.15, 0.25, 0.30),
        listOf(listOf(1.0, 0.3, 0.1), listOf(0.3, 1.0, 0.4), listOf(0.1, 0.4, 1.0)),
    )

    private fun assertNear(expected: Double, actual: Double, eps: Double = 1e-9) =
        assertTrue(abs(expected - actual) < eps, "expected $expected, got $actual")

    private fun unit(i: Int): Vec = List(3) { if (it == i) 1.0 else 0.0 }

    @Test
    fun scenarios() {
        val p = listOf(0.2, 0.5, 0.3)
        val k1 = listOf(-0.10, 0.10, 0.20)
        val k2 = listOf(0.20, 0.05, -0.05)
        assertNear(0.09, mean(k1, p))
        assertNear(-0.009, covariance(k1, k2, p))
        val rho = correlation(k1, k2, p)
        for (w in listOf(0.0, 0.3, 0.5, 1.2)) {
            val kV = k1.indices.map { w * k1[it] + (1 - w) * k2[it] }
            assertNear(twoAssetStd(w, stdDev(k1, p), stdDev(k2, p), rho), stdDev(kV, p))
            assertNear(twoAssetMean(w, mean(k1, p), mean(k2, p)), mean(kV, p))
        }
    }

    @Test
    fun twoAssets() {
        for (rho in listOf(-0.9, -0.5, 0.0, 0.3)) {
            val w = minVarianceWeight(0.10, 0.20, rho)
            val s = twoAssetStd(w, 0.10, 0.20, rho)
            assertTrue(s < twoAssetStd(w - 0.01, 0.10, 0.20, rho))
            assertTrue(s < twoAssetStd(w + 0.01, 0.10, 0.20, rho))
        }
        assertNear(0.0, twoAssetStd(2.0 / 3, 0.10, 0.20, -1.0), 1e-7)
    }

    @Test
    fun inverseTimesCovariance() {
        val covInv = inverse(cov)
        for (i in 0 until 3) {
            val row = vecMat(unit(i), cov)
            for (j in 0 until 3) {
                val column = listOf(covInv[0][j], covInv[1][j], covInv[2][j])
                assertNear(if (i == j) 1.0 else 0.0, dot(row, column))
            }
        }
    }

    @Test
    fun frontier() {
        val wMin = minVariancePortfolio(cov)
        assertNear(1.0, sum(wMin))
        assertNear(wMin[0], minVarianceLine(means, cov, portfolioMean(wMin, means))[0])
        for (mu in listOf(0.12, 0.15, 0.25)) {
            val w = minVarianceLine(means, cov, mu)
            assertNear(1.0, sum(w))
            assertNear(mu, portfolioMean(w, means))
            assertTrue(portfolioStd(w, cov) >= portfolioStd(wMin, cov))
            // Any other portfolio with the same mean is riskier: move along (1, -2, 1).
            val moved = listOf(w[0] + 0.01, w[1] - 0.02, w[2] + 0.01)
            assertTrue(portfolioStd(w, cov) < portfolioStd(moved, cov))
        }
    }

    @Test
    fun capm() {
        val wMarket = marketPortfolio(means, cov, 0.05)
        val muM = portfolioMean(wMarket, means)
        val sigmaM = portfolioStd(wMarket, cov)
        assertNear(1.0, sum(wMarket))
        assertNear(1.0, beta(wMarket, wMarket, cov))
        assertNear(muM, capitalMarketLine(sigmaM, 0.05, muM, sigmaM))
        // The market portfolio is on the frontier, and the CML is tangent there.
        assertNear(sigmaM, portfolioStd(minVarianceLine(means, cov, muM), cov))
        for (i in 0 until 3) {
            assertNear(means[i], securityMarketLine(beta(unit(i), wMarket, cov), 0.05, muM))
        }
    }
}
