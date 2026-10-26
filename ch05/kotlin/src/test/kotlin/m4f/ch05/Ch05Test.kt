package m4f.ch05

import kotlin.math.abs
import kotlin.test.Test
import kotlin.test.assertTrue

class Ch05Test {
    private fun near(a: Double, b: Double, eps: Double = 1e-9) = assertTrue(abs(a - b) < eps, "$a != $b")
    private fun unit(i: Int): Vec = List(3) { if (it == i) 1.0 else 0.0 }

    private val M = listOf(0.10, 0.15, 0.20)
    private val C = covarianceMatrix(listOf(0.15, 0.25, 0.30),
        listOf(listOf(1.0, 0.3, 0.1), listOf(0.3, 1.0, 0.4), listOf(0.1, 0.4, 1.0)))

    @Test fun scenarios() {
        val p = listOf(0.2, 0.5, 0.3)
        val K1 = listOf(-0.10, 0.10, 0.20)
        val K2 = listOf(0.20, 0.05, -0.05)
        near(mean(K1, p), 0.09)
        near(covariance(K1, K2, p), -0.009)
        val rho = correlation(K1, K2, p)
        for (w in listOf(0.0, 0.3, 0.5, 1.2)) {
            val KV = K1.indices.map { w * K1[it] + (1 - w) * K2[it] }
            near(stdDev(KV, p), twoAssetStd(w, stdDev(K1, p), stdDev(K2, p), rho))
            near(mean(KV, p), twoAssetMean(w, mean(K1, p), mean(K2, p)))
        }
    }

    @Test fun twoAssets() {
        for (r in listOf(-0.9, -0.5, 0.0, 0.3)) {
            val w = minVarianceWeight(0.10, 0.20, r)
            val s = twoAssetStd(w, 0.10, 0.20, r)
            assertTrue(s < twoAssetStd(w - 0.01, 0.10, 0.20, r) && s < twoAssetStd(w + 0.01, 0.10, 0.20, r))
        }
        near(twoAssetStd(2.0 / 3, 0.10, 0.20, -1.0), 0.0, 1e-7)
    }

    @Test fun inverseTimesC() {
        val Ci = inverse(C)
        for (i in 0 until 3) {
            val row = vecMat(unit(i), C)
            for (j in 0 until 3) near(dot(row, listOf(Ci[0][j], Ci[1][j], Ci[2][j])), if (i == j) 1.0 else 0.0)
        }
    }

    @Test fun frontier() {
        val wmv = minVariancePortfolio(C)
        near(sum(wmv), 1.0)
        near(minVarianceLine(M, C, portfolioMean(wmv, M))[0], wmv[0])
        for (mu in listOf(0.12, 0.15, 0.25)) {
            val w = minVarianceLine(M, C, mu)
            near(sum(w), 1.0)
            near(portfolioMean(w, M), mu)
            assertTrue(portfolioStd(w, C) >= portfolioStd(wmv, C))
            assertTrue(portfolioStd(w, C) < portfolioStd(listOf(w[0] + 0.01, w[1] - 0.02, w[2] + 0.01), C))
        }
    }

    @Test fun capm() {
        val wM = marketPortfolio(M, C, 0.05)
        val muM = portfolioMean(wM, M)
        val sM = portfolioStd(wM, C)
        near(sum(wM), 1.0)
        near(beta(wM, wM, C), 1.0)
        near(capitalMarketLine(sM, 0.05, muM, sM), muM)
        near(portfolioStd(minVarianceLine(M, C, muM), C), sM)
        for (i in 0 until 3) near(securityMarketLine(beta(unit(i), wM, C), 0.05, muM), M[i])
    }
}
