// Sections 5.3-5.4: several securities and the Capital Asset Pricing Model.
//
// Weights w, expected returns m and the covariance matrix c are plain lists; u = (1, ..., 1).
// The formulas are the book's matrix forms:
//   mu_V = w m^T, sigma_V^2 = w C w^T.
package m4f.ch05

import kotlin.math.abs
import kotlin.math.sqrt

typealias Vec = List<Double>
typealias Mat = List<List<Double>>

// Linear algebra kept deliberately plain so all three languages compute alike.

/** Inner product of [a] and [b]. */
fun dot(a: Vec, b: Vec): Double {
    var s = 0.0
    for (i in a.indices) {
        s += a[i] * b[i]
    }
    return s
}

/** Sum of the entries, added left to right. */
fun sum(a: Vec): Double {
    var s = 0.0
    for (x in a) {
        s += x
    }
    return s
}

/**
 * v M, for a row vector [v]. Row by row, so the matrix is read in memory order and no column is
 * copied; each out[j] still sums v[i] m[i][j] in order of i.
 */
fun vecMat(v: Vec, m: Mat): Vec {
    val out = DoubleArray(m[0].size)
    for (i in m.indices) {
        val vi = v[i]
        val row = m[i]
        for (j in out.indices) {
            out[j] += vi * row[j]
        }
    }
    return out.asList()
}

/** Gauss-Jordan elimination with partial pivoting. */
fun inverse(m: Mat): Mat {
    val n = m.size
    val a = Array(n) { i ->
        DoubleArray(2 * n) { j ->
            when {
                j < n -> m[i][j]
                j - n == i -> 1.0
                else -> 0.0
            }
        }
    }
    for (c in 0 until n) {
        var pivot = c
        for (r in c + 1 until n) {
            if (abs(a[r][c]) > abs(a[pivot][c])) pivot = r
        }
        val tmp = a[c]
        a[c] = a[pivot]
        a[pivot] = tmp
        // Columns left of c are already unit columns (exact zeros off the diagonal), so each row
        // operation starts at column c.
        val d = a[c][c]
        for (j in c until 2 * n) {
            a[c][j] /= d
        }
        for (r in 0 until n) {
            if (r == c) continue
            val k = a[r][c]
            if (k == 0.0) continue
            for (j in c until 2 * n) {
                a[r][j] -= k * a[c][j]
            }
        }
    }
    return a.map { row -> row.slice(n until 2 * n) }
}

/**
 * Solves C x = b for each right-hand side b in [rhs], for a symmetric C. Uses a Cholesky
 * factorisation C = L L^T (n^3/6 multiply-adds, then n^2 per right-hand side) when C is positive
 * definite, as a covariance matrix of non-redundant assets is; otherwise falls back to the
 * explicit inverse. For symmetric C, x = b C^-1 read as a row vector, so this replaces
 * vecMat(b, inverse(c)) without forming the inverse.
 */
fun solveSymmetric(c: Mat, rhs: List<Vec>): List<Vec> {
    val n = c.size
    val l = DoubleArray(n * n) // Row-major lower triangle.
    for (i in 0 until n) {
        for (j in 0..i) {
            var sum = c[i][j]
            for (k in 0 until j) {
                sum -= l[i * n + k] * l[j * n + k]
            }
            if (i == j) {
                if (!(sum > 0)) {
                    val inv = inverse(c)
                    return rhs.map { vecMat(it, inv) }
                }
                l[i * n + i] = sqrt(sum)
            } else {
                l[i * n + j] = sum / l[j * n + j]
            }
        }
    }
    return rhs.map { b ->
        val x = b.toDoubleArray()
        for (i in 0 until n) { // L z = b.
            var sum = x[i]
            for (k in 0 until i) {
                sum -= l[i * n + k] * x[k]
            }
            x[i] = sum / l[i * n + i]
        }
        for (i in n - 1 downTo 0) { // L^T x = z.
            var sum = x[i]
            for (k in i + 1 until n) {
                sum -= l[k * n + i] * x[k]
            }
            x[i] = sum / l[i * n + i]
        }
        x.asList()
    }
}

/** c_ij = rho_ij sigma_i sigma_j. */
fun covarianceMatrix(sigma: Vec, rho: Mat): Mat =
    sigma.indices.map { i -> sigma.indices.map { j -> rho[i][j] * sigma[i] * sigma[j] } }

// Section 5.3.1: risk and expected return on a portfolio.

/** mu_V = w m^T. */
fun portfolioMean(w: Vec, m: Vec): Double = dot(w, m)

/** sigma_V = sqrt(w C w^T). */
fun portfolioStd(w: Vec, cov: Mat): Double = sqrt(dot(vecMat(w, cov), w))

// Section 5.3.2: efficient frontier.

/** w = u C^-1 / (u C^-1 u^T). */
fun minVariancePortfolio(cov: Mat): Vec {
    val uCovInv = solveSymmetric(cov, listOf(List(cov.size) { 1.0 }))[0]
    val s = sum(uCovInv)
    return uCovInv.map { it / s }
}

/**
 * Minimum variance portfolio with expected return [muV]. With a = u C^-1 u^T, b = u C^-1 m^T,
 * c = m C^-1 m^T:
 *     w = ((c - b muV) u C^-1 + (a muV - b) m C^-1) / (a c - b^2)
 */
fun minVarianceLine(m: Vec, cov: Mat, muV: Double): Vec {
    val (uCovInv, mCovInv) = solveSymmetric(cov, listOf(List(cov.size) { 1.0 }, m))
    val a = sum(uCovInv)
    val b = dot(uCovInv, m)
    val c = dot(mCovInv, m)
    val det = a * c - b * b
    return m.indices.map { ((c - b * muV) * uCovInv[it] + (a * muV - b) * mCovInv[it]) / det }
}

// Section 5.4: Capital Asset Pricing Model, risk-free return r.

/** Tangency portfolio: w_M = (m - r u) C^-1 / ((m - r u) C^-1 u^T). */
fun marketPortfolio(m: Vec, cov: Mat, r: Double): Vec {
    val w = solveSymmetric(cov, listOf(m.map { it - r }))[0]
    val s = sum(w)
    return w.map { it / s }
}

/** Section 5.4.1: mu_V = r + (mu_M - r) / sigma_M * sigma_V. */
fun capitalMarketLine(sigmaV: Double, r: Double, muM: Double, sigmaM: Double): Double =
    r + (muM - r) / sigmaM * sigmaV

/** Section 5.4.2: beta_V = Cov(K_V, K_M) / sigma_M^2 = w C w_M^T / (w_M C w_M^T). */
fun beta(w: Vec, wMarket: Vec, cov: Mat): Double {
    // C is symmetric, so w_M C = (C w_M^T)^T.
    val covWMarket = vecMat(wMarket, cov)
    return dot(covWMarket, w) / dot(covWMarket, wMarket)
}

/** Section 5.4.3: mu_V = r + (mu_M - r) beta_V. */
fun securityMarketLine(betaV: Double, r: Double, muM: Double): Double = r + (muM - r) * betaV
