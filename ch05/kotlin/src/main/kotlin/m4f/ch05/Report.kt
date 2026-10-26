package m4f.ch05

import java.math.BigDecimal
import java.math.RoundingMode

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations.
private fun f(v: Double): String = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

private fun fl(vs: List<Double>): String = vs.joinToString(", ", "[", "]") { f(it) }

private fun StringBuilder.line(s: String) = append(s).append('\n')

private fun unit(i: Int, n: Int): Vec = List(n) { if (it == i) 1.0 else 0.0 }

fun report(): String = buildString {
    line("Chapter 5: Portfolio Management (illustrative numbers, book formulas)")

    line("\n== 5.1 Risk: scenarios with p = {0.2, 0.5, 0.3} ==")
    val p = listOf(0.2, 0.5, 0.3)
    val K1 = listOf(-0.10, 0.10, 0.20)
    val K2 = listOf(0.20, 0.05, -0.05)
    line("K1=${fl(K1)}: mu=${f(mean(K1, p))} sigma=${f(stdDev(K1, p))}")
    line("K2=${fl(K2)}: mu=${f(mean(K2, p))} sigma=${f(stdDev(K2, p))}")
    line("Cov(K1,K2)=${f(covariance(K1, K2, p))} rho=${f(correlation(K1, K2, p))}")
    val KV = K1.indices.map { 0.5 * K1[it] + 0.5 * K2[it] }
    val rho = correlation(K1, K2, p)
    line("Half and half: sigma from scenarios=${f(stdDev(KV, p))}, " +
        "from formula=${f(twoAssetStd(0.5, stdDev(K1, p), stdDev(K2, p), rho))}")

    line("\n== 5.2.1 Two securities: mu1=0.08 sigma1=0.10, mu2=0.12 sigma2=0.20 ==")
    for (r in listOf(-1.0, -0.5, 0.0, 0.5, 1.0)) {
        val w = minVarianceWeight(0.10, 0.20, r)
        line("rho=${f(r)}: min variance w1=${f(w)} mu=${f(twoAssetMean(w, 0.08, 0.12))} " +
            "sigma=${f(twoAssetStd(w, 0.10, 0.20, r))}; w1=0.4: sigma=${f(twoAssetStd(0.4, 0.10, 0.20, r))}")
    }

    line("\n== 5.3.1 Several securities: m=(0.10, 0.15, 0.20) sigma=(0.15, 0.25, 0.30) ==")
    val m = listOf(0.10, 0.15, 0.20)
    val C = covarianceMatrix(listOf(0.15, 0.25, 0.30),
        listOf(listOf(1.0, 0.3, 0.1), listOf(0.3, 1.0, 0.4), listOf(0.1, 0.4, 1.0)))
    line("rho12=0.3 rho13=0.1 rho23=0.4")
    val w = listOf(0.4, 0.3, 0.3)
    line("w=${fl(w)}: mu=${f(portfolioMean(w, m))} sigma=${f(portfolioStd(w, C))}")

    line("\n== 5.3.2 Efficient frontier ==")
    val wmv = minVariancePortfolio(C)
    line("Minimum variance portfolio w=${fl(wmv)} mu=${f(portfolioMean(wmv, m))} sigma=${f(portfolioStd(wmv, C))}")
    for (mu in listOf(0.15, 0.18, 0.21)) {
        val wl = minVarianceLine(m, C, mu)
        line("mu_V=${f(mu)}: w=${fl(wl)} sigma=${f(portfolioStd(wl, C))}")
    }

    line("\n== 5.4.1 Capital market line: R=0.05 ==")
    val wM = marketPortfolio(m, C, 0.05)
    val muM = portfolioMean(wM, m)
    val sM = portfolioStd(wM, C)
    line("Market portfolio w_M=${fl(wM)} mu_M=${f(muM)} sigma_M=${f(sM)} slope=${f((muM - 0.05) / sM)}")
    line("On the CML at sigma=0.15: mu=${f(capitalMarketLine(0.15, 0.05, muM, sM))}")

    line("\n== 5.4.2 Beta factor: beta_V = Cov(K_V, K_M) / sigma_M^2 ==")
    val betas = (0 until 3).map { beta(unit(it, 3), wM, C) }
    line("beta=${fl(betas)}")
    var wb = 0.0
    for (i in 0 until 3) wb += w[i] * betas[i]
    line("w=${fl(w)}: beta_V=${f(beta(w, wM, C))} sum w_i beta_i=${f(wb)}")

    line("\n== 5.4.3 Security market line: mu_V = R + (mu_M - R) beta_V ==")
    for (i in 0 until 3)
        line("security ${i + 1}: beta=${f(betas[i])} SML mu=${f(securityMarketLine(betas[i], 0.05, muM))} " +
            "actual mu=${f(m[i])}")
}
