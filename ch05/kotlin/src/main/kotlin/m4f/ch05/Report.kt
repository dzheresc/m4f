package m4f.ch05

import java.math.BigDecimal
import java.math.RoundingMode

// Round-half-even on the exact binary value, like C++ std::format and Python, so the report is
// byte-identical across the three implementations. Adding 0.0 turns -0.0 into 0.0.
private fun fmt(v: Double): String =
    BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

/** "[a, b, c]" with each entry formatted by [fmt]. */
private fun fmtList(values: List<Double>): String = values.joinToString(", ", "[", "]") { fmt(it) }

private fun StringBuilder.line(s: String) {
    append(s).append('\n')
}

/** The [i]-th unit vector of length [n]. */
private fun unit(i: Int, n: Int): Vec = List(n) { if (it == i) 1.0 else 0.0 }

// The three-security market shared by sections 5.3 and 5.4.
private val MEANS = listOf(0.10, 0.15, 0.20)
private val WEIGHTS = listOf(0.4, 0.3, 0.3)
private const val RISK_FREE = 0.05

private fun marketCovariance(): Mat = covarianceMatrix(
    listOf(0.15, 0.25, 0.30),
    listOf(listOf(1.0, 0.3, 0.1), listOf(0.3, 1.0, 0.4), listOf(0.1, 0.4, 1.0)),
)

/** Section 5.1. */
private fun StringBuilder.riskSection() {
    line("\n== 5.1 Risk: scenarios with p = {0.2, 0.5, 0.3} ==")
    val p = listOf(0.2, 0.5, 0.3)
    val k1 = listOf(-0.10, 0.10, 0.20)
    val k2 = listOf(0.20, 0.05, -0.05)
    line("K1=${fmtList(k1)}: mu=${fmt(mean(k1, p))} sigma=${fmt(stdDev(k1, p))}")
    line("K2=${fmtList(k2)}: mu=${fmt(mean(k2, p))} sigma=${fmt(stdDev(k2, p))}")
    line("Cov(K1,K2)=${fmt(covariance(k1, k2, p))} rho=${fmt(correlation(k1, k2, p))}")
    val kV = k1.indices.map { 0.5 * k1[it] + 0.5 * k2[it] }
    val rho = correlation(k1, k2, p)
    val formula = twoAssetStd(0.5, stdDev(k1, p), stdDev(k2, p), rho)
    line(
        "Half and half: sigma from scenarios=${fmt(stdDev(kV, p))}, " +
            "from formula=${fmt(formula)}",
    )
}

/** Section 5.2.1. */
private fun StringBuilder.twoSecuritiesSection() {
    line("\n== 5.2.1 Two securities: mu1=0.08 sigma1=0.10, mu2=0.12 sigma2=0.20 ==")
    for (rho in listOf(-1.0, -0.5, 0.0, 0.5, 1.0)) {
        val w = minVarianceWeight(0.10, 0.20, rho)
        line(
            "rho=${fmt(rho)}: min variance w1=${fmt(w)} mu=${fmt(twoAssetMean(w, 0.08, 0.12))} " +
                "sigma=${fmt(twoAssetStd(w, 0.10, 0.20, rho))}; " +
                "w1=0.4: sigma=${fmt(twoAssetStd(0.4, 0.10, 0.20, rho))}",
        )
    }
}

/** Section 5.3.1. */
private fun StringBuilder.severalSecuritiesSection(cov: Mat) {
    line("\n== 5.3.1 Several securities: m=(0.10, 0.15, 0.20) sigma=(0.15, 0.25, 0.30) ==")
    line("rho12=0.3 rho13=0.1 rho23=0.4")
    line(
        "w=${fmtList(WEIGHTS)}: mu=${fmt(portfolioMean(WEIGHTS, MEANS))} " +
            "sigma=${fmt(portfolioStd(WEIGHTS, cov))}",
    )
}

/** Section 5.3.2. */
private fun StringBuilder.efficientFrontierSection(cov: Mat) {
    line("\n== 5.3.2 Efficient frontier ==")
    val wMin = minVariancePortfolio(cov)
    line(
        "Minimum variance portfolio w=${fmtList(wMin)} mu=${fmt(portfolioMean(wMin, MEANS))} " +
            "sigma=${fmt(portfolioStd(wMin, cov))}",
    )
    for (mu in listOf(0.15, 0.18, 0.21)) {
        val w = minVarianceLine(MEANS, cov, mu)
        line("mu_V=${fmt(mu)}: w=${fmtList(w)} sigma=${fmt(portfolioStd(w, cov))}")
    }
}

/** Sections 5.4.1-5.4.3. */
private fun StringBuilder.capmSection(cov: Mat) {
    line("\n== 5.4.1 Capital market line: R=0.05 ==")
    val wMarket = marketPortfolio(MEANS, cov, RISK_FREE)
    val muM = portfolioMean(wMarket, MEANS)
    val sigmaM = portfolioStd(wMarket, cov)
    line(
        "Market portfolio w_M=${fmtList(wMarket)} mu_M=${fmt(muM)} sigma_M=${fmt(sigmaM)} " +
            "slope=${fmt((muM - RISK_FREE) / sigmaM)}",
    )
    line("On the CML at sigma=0.15: mu=${fmt(capitalMarketLine(0.15, RISK_FREE, muM, sigmaM))}")

    line("\n== 5.4.2 Beta factor: beta_V = Cov(K_V, K_M) / sigma_M^2 ==")
    val betas = (0 until 3).map { beta(unit(it, 3), wMarket, cov) }
    line("beta=${fmtList(betas)}")
    var weightedBeta = 0.0
    for (i in 0 until 3) {
        weightedBeta += WEIGHTS[i] * betas[i]
    }
    line(
        "w=${fmtList(WEIGHTS)}: beta_V=${fmt(beta(WEIGHTS, wMarket, cov))} " +
            "sum w_i beta_i=${fmt(weightedBeta)}",
    )

    line("\n== 5.4.3 Security market line: mu_V = R + (mu_M - R) beta_V ==")
    for (i in 0 until 3) {
        val sml = securityMarketLine(betas[i], RISK_FREE, muM)
        line(
            "security ${i + 1}: beta=${fmt(betas[i])} SML mu=${fmt(sml)} " +
                "actual mu=${fmt(MEANS[i])}",
        )
    }
}

/** The full chapter 5 report. */
fun report(): String = buildString {
    line("Chapter 5: Portfolio Management (illustrative numbers, book formulas)")
    val cov = marketCovariance()
    riskSection()
    twoSecuritiesSection()
    severalSecuritiesSection(cov)
    efficientFrontierSection(cov)
    capmSection(cov)
}
