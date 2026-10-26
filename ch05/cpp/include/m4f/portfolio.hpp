#pragma once
// Sections 5.3-5.4: several securities and the Capital Asset Pricing Model.
//
// Weights w, expected returns m and the covariance matrix C are plain vectors;
// u = (1, ..., 1). The formulas are the book's matrix forms:
//   mu_V = w m^T, sigma_V^2 = w C w^T.
#include "m4f/risk.hpp"

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace m4f::ch05 {

using Mat = std::vector<Vec>;

// Linear algebra kept deliberately plain so all three languages compute alike.
inline double dot(const Vec& a, const Vec& b) {
    double s = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) s += a[i] * b[i];
    return s;
}

inline double sum(const Vec& a) {
    double s = 0.0;
    for (double x : a) s += x;
    return s;
}

// v M, for a row vector v
inline Vec vecMat(const Vec& v, const Mat& M) {
    Vec out;
    for (std::size_t j = 0; j < M[0].size(); ++j) {
        Vec col;
        for (std::size_t i = 0; i < M.size(); ++i) col.push_back(M[i][j]);
        out.push_back(dot(v, col));
    }
    return out;
}

// Gauss-Jordan elimination with partial pivoting
inline Mat inverse(const Mat& M) {
    const std::size_t n = M.size();
    Mat a(n, Vec(2 * n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) a[i][j] = M[i][j];
        a[i][n + i] = 1.0;
    }
    for (std::size_t c = 0; c < n; ++c) {
        std::size_t piv = c;
        for (std::size_t r = c + 1; r < n; ++r)
            if (std::abs(a[r][c]) > std::abs(a[piv][c])) piv = r;
        std::swap(a[c], a[piv]);
        const double d = a[c][c];
        for (double& x : a[c]) x /= d;
        for (std::size_t r = 0; r < n; ++r) {
            if (r == c) continue;
            const double k = a[r][c];
            for (std::size_t j = 0; j < 2 * n; ++j) a[r][j] -= k * a[c][j];
        }
    }
    Mat inv(n);
    for (std::size_t i = 0; i < n; ++i) inv[i].assign(a[i].begin() + n, a[i].end());
    return inv;
}

// c_ij = rho_ij sigma_i sigma_j
inline Mat covarianceMatrix(const Vec& sigma, const Mat& rho) {
    const std::size_t n = sigma.size();
    Mat C(n, Vec(n));
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) C[i][j] = rho[i][j] * sigma[i] * sigma[j];
    return C;
}

// 5.3.1 Risk and expected return on a portfolio
inline double portfolioMean(const Vec& w, const Vec& m) { return dot(w, m); }
inline double portfolioStd(const Vec& w, const Mat& C) { return std::sqrt(dot(vecMat(w, C), w)); }

// 5.3.2 Efficient frontier
// w = u C^-1 / (u C^-1 u^T)
inline Vec minVariancePortfolio(const Mat& C) {
    Vec w = vecMat(Vec(C.size(), 1.0), inverse(C));
    const double s = sum(w);
    for (double& x : w) x /= s;
    return w;
}

// Minimum variance portfolio with expected return muV. With a = u C^-1 u^T,
// b = u C^-1 m^T, c = m C^-1 m^T:
//   w = ((c - b muV) u C^-1 + (a muV - b) m C^-1) / (a c - b^2)
inline Vec minVarianceLine(const Vec& m, const Mat& C, double muV) {
    const Mat Ci = inverse(C);
    const Vec uCi = vecMat(Vec(C.size(), 1.0), Ci), mCi = vecMat(m, Ci);
    const double a = sum(uCi), b = dot(uCi, m), c = dot(mCi, m);
    const double det = a * c - b * b;
    Vec w;
    for (std::size_t i = 0; i < m.size(); ++i) w.push_back(((c - b * muV) * uCi[i] + (a * muV - b) * mCi[i]) / det);
    return w;
}

// 5.4 Capital Asset Pricing Model, risk-free return R
// Tangency portfolio: w_M = (m - R u) C^-1 / ((m - R u) C^-1 u^T)
inline Vec marketPortfolio(const Vec& m, const Mat& C, double R) {
    Vec ex;
    for (double x : m) ex.push_back(x - R);
    Vec w = vecMat(ex, inverse(C));
    const double s = sum(w);
    for (double& x : w) x /= s;
    return w;
}

// 5.4.1 mu_V = R + (mu_M - R) / sigma_M * sigma_V
inline double capitalMarketLine(double sigmaV, double R, double muM, double sigmaM) {
    return R + (muM - R) / sigmaM * sigmaV;
}

// 5.4.2 beta_V = Cov(K_V, K_M) / sigma_M^2 = w C w_M^T / (w_M C w_M^T)
inline double beta(const Vec& w, const Vec& wM, const Mat& C) {
    const Vec CwM = vecMat(wM, C);  // C is symmetric, so w_M C = (C w_M^T)^T
    return dot(CwM, w) / dot(CwM, wM);
}

// 5.4.3 mu_V = R + (mu_M - R) beta_V
inline double securityMarketLine(double b, double R, double muM) { return R + (muM - R) * b; }

}  // namespace m4f::ch05
