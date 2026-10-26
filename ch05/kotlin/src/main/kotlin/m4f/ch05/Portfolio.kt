package m4f.ch05

import kotlin.math.abs
import kotlin.math.sqrt

// Sections 5.3-5.4: several securities and the Capital Asset Pricing Model.
//
// Weights w, expected returns m and the covariance matrix C are plain lists;
// u = (1, ..., 1). The formulas are the book's matrix forms:
//   mu_V = w m^T, sigma_V^2 = w C w^T.

typealias Vec = List<Double>
typealias Mat = List<List<Double>>

// Linear algebra kept deliberately plain so all three languages compute alike.
fun dot(a: Vec, b: Vec): Double {
    var s = 0.0
    for (i in a.indices) s += a[i] * b[i]
    return s
}

fun sum(a: Vec): Double {
    var s = 0.0
    for (x in a) s += x
    return s
}

/** v M, for a row vector v */
fun vecMat(v: Vec, M: Mat): Vec = M[0].indices.map { j -> dot(v, M.indices.map { i -> M[i][j] }) }

/** Gauss-Jordan elimination with partial pivoting */
fun inverse(M: Mat): Mat {
    val n = M.size
    val a = Array(n) { i -> DoubleArray(2 * n) { j -> if (j < n) M[i][j] else if (j - n == i) 1.0 else 0.0 } }
    for (c in 0 until n) {
        var piv = c
        for (r in c + 1 until n) if (abs(a[r][c]) > abs(a[piv][c])) piv = r
        val tmp = a[c]; a[c] = a[piv]; a[piv] = tmp
        val d = a[c][c]
        for (j in 0 until 2 * n) a[c][j] /= d
        for (r in 0 until n) {
            if (r == c) continue
            val k = a[r][c]
            for (j in 0 until 2 * n) a[r][j] -= k * a[c][j]
        }
    }
    return a.map { row -> row.slice(n until 2 * n) }
}

/** c_ij = rho_ij sigma_i sigma_j */
fun covarianceMatrix(sigma: Vec, rho: Mat): Mat =
    sigma.indices.map { i -> sigma.indices.map { j -> rho[i][j] * sigma[i] * sigma[j] } }

// 5.3.1 Risk and expected return on a portfolio
fun portfolioMean(w: Vec, m: Vec): Double = dot(w, m)
fun portfolioStd(w: Vec, C: Mat): Double = sqrt(dot(vecMat(w, C), w))

// 5.3.2 Efficient frontier
/** w = u C^-1 / (u C^-1 u^T) */
fun minVariancePortfolio(C: Mat): Vec {
    val uCi = vecMat(List(C.size) { 1.0 }, inverse(C))
    val s = sum(uCi)
    return uCi.map { it / s }
}

/** Minimum variance portfolio with expected return muV. With a = u C^-1 u^T,
 *  b = u C^-1 m^T, c = m C^-1 m^T:
 *    w = ((c - b muV) u C^-1 + (a muV - b) m C^-1) / (a c - b^2) */
fun minVarianceLine(m: Vec, C: Mat, muV: Double): Vec {
    val Ci = inverse(C)
    val uCi = vecMat(List(C.size) { 1.0 }, Ci)
    val mCi = vecMat(m, Ci)
    val a = sum(uCi)
    val b = dot(uCi, m)
    val c = dot(mCi, m)
    val det = a * c - b * b
    return m.indices.map { ((c - b * muV) * uCi[it] + (a * muV - b) * mCi[it]) / det }
}

// 5.4 Capital Asset Pricing Model, risk-free return R
/** Tangency portfolio: w_M = (m - R u) C^-1 / ((m - R u) C^-1 u^T) */
fun marketPortfolio(m: Vec, C: Mat, R: Double): Vec {
    val w = vecMat(m.map { it - R }, inverse(C))
    val s = sum(w)
    return w.map { it / s }
}

/** 5.4.1 mu_V = R + (mu_M - R) / sigma_M * sigma_V */
fun capitalMarketLine(sigmaV: Double, R: Double, muM: Double, sigmaM: Double): Double =
    R + (muM - R) / sigmaM * sigmaV

/** 5.4.2 beta_V = Cov(K_V, K_M) / sigma_M^2 = w C w_M^T / (w_M C w_M^T) */
fun beta(w: Vec, wM: Vec, C: Mat): Double {
    val CwM = vecMat(wM, C)  // C is symmetric, so w_M C = (C w_M^T)^T
    return dot(CwM, w) / dot(CwM, wM)
}

/** 5.4.3 mu_V = R + (mu_M - R) beta_V */
fun securityMarketLine(b: Double, R: Double, muM: Double): Double = R + (muM - R) * b
