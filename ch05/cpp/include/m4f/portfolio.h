// Sections 5.3-5.4: several securities and the Capital Asset Pricing Model.
//
// Weights w, expected returns m and the covariance matrix c are plain vectors;
// u = (1, ..., 1). The formulas are the book's matrix forms:
//   mu_V = w m^T, sigma_V^2 = w C w^T.
#ifndef M4F_CH05_PORTFOLIO_H_
#define M4F_CH05_PORTFOLIO_H_

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "m4f/risk.h"

namespace m4f::ch05 {

using Mat = std::vector<Vec>;

// Linear algebra kept deliberately plain so all three languages compute alike.
inline double Dot(const Vec& a, const Vec& b) {
  double s = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) s += a[i] * b[i];
  return s;
}

inline double Sum(const Vec& a) {
  double s = 0.0;
  for (double x : a) s += x;
  return s;
}

// v M, for a row vector v.
inline Vec VecMat(const Vec& v, const Mat& m) {
  Vec out;
  for (std::size_t j = 0; j < m[0].size(); ++j) {
    Vec col;
    for (std::size_t i = 0; i < m.size(); ++i) col.push_back(m[i][j]);
    out.push_back(Dot(v, col));
  }
  return out;
}

// Gauss-Jordan elimination with partial pivoting.
inline Mat Inverse(const Mat& m) {
  const std::size_t n = m.size();
  Mat a(n, Vec(2 * n, 0.0));
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) a[i][j] = m[i][j];
    a[i][n + i] = 1.0;
  }
  for (std::size_t c = 0; c < n; ++c) {
    std::size_t pivot = c;
    for (std::size_t r = c + 1; r < n; ++r) {
      if (std::abs(a[r][c]) > std::abs(a[pivot][c])) pivot = r;
    }
    std::swap(a[c], a[pivot]);
    const double d = a[c][c];
    for (double& x : a[c]) x /= d;
    for (std::size_t r = 0; r < n; ++r) {
      if (r == c) continue;
      const double k = a[r][c];
      for (std::size_t j = 0; j < 2 * n; ++j) a[r][j] -= k * a[c][j];
    }
  }
  Mat inv(n);
  for (std::size_t i = 0; i < n; ++i) {
    inv[i].assign(a[i].begin() + n, a[i].end());
  }
  return inv;
}

// c_ij = rho_ij sigma_i sigma_j.
inline Mat CovarianceMatrix(const Vec& sigma, const Mat& rho) {
  const std::size_t n = sigma.size();
  Mat c(n, Vec(n));
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j)
      c[i][j] = rho[i][j] * sigma[i] * sigma[j];
  }
  return c;
}

// Section 5.3.1: risk and expected return on a portfolio.
inline double PortfolioMean(const Vec& w, const Vec& m) { return Dot(w, m); }

inline double PortfolioStd(const Vec& w, const Mat& c) {
  return std::sqrt(Dot(VecMat(w, c), w));
}

// Section 5.3.2: efficient frontier. w = u C^-1 / (u C^-1 u^T).
inline Vec MinVariancePortfolio(const Mat& c) {
  Vec w = VecMat(Vec(c.size(), 1.0), Inverse(c));
  const double s = Sum(w);
  for (double& x : w) x /= s;
  return w;
}

// Minimum variance portfolio with expected return mu_v. With a = u C^-1 u^T,
// b = u C^-1 m^T, c = m C^-1 m^T:
//   w = ((c - b mu_v) u C^-1 + (a mu_v - b) m C^-1) / (a c - b^2)
inline Vec MinVarianceLine(const Vec& m, const Mat& cov, double mu_v) {
  const Mat cov_inv = Inverse(cov);
  const Vec u_cov_inv = VecMat(Vec(cov.size(), 1.0), cov_inv);
  const Vec m_cov_inv = VecMat(m, cov_inv);
  const double a = Sum(u_cov_inv);
  const double b = Dot(u_cov_inv, m);
  const double c = Dot(m_cov_inv, m);
  const double det = a * c - b * b;
  Vec w;
  for (std::size_t i = 0; i < m.size(); ++i) {
    w.push_back(
        ((c - b * mu_v) * u_cov_inv[i] + (a * mu_v - b) * m_cov_inv[i]) / det);
  }
  return w;
}

// Section 5.4: Capital Asset Pricing Model, risk-free return r.
// Tangency portfolio: w_M = (m - r u) C^-1 / ((m - r u) C^-1 u^T).
inline Vec MarketPortfolio(const Vec& m, const Mat& c, double r) {
  Vec excess;
  for (double x : m) excess.push_back(x - r);
  Vec w = VecMat(excess, Inverse(c));
  const double s = Sum(w);
  for (double& x : w) x /= s;
  return w;
}

// Section 5.4.1: mu_V = r + (mu_M - r) / sigma_M * sigma_V.
inline double CapitalMarketLine(double sigma_v, double r, double mu_m,
                                double sigma_m) {
  return r + (mu_m - r) / sigma_m * sigma_v;
}

// Section 5.4.2: beta_V = Cov(K_V, K_M) / sigma_M^2 = w C w_M^T / (w_M C
// w_M^T).
inline double Beta(const Vec& w, const Vec& w_market, const Mat& c) {
  // C is symmetric, so w_M C = (C w_M^T)^T.
  const Vec c_w_market = VecMat(w_market, c);
  return Dot(c_w_market, w) / Dot(c_w_market, w_market);
}

// Section 5.4.3: mu_V = r + (mu_M - r) beta_V.
inline double SecurityMarketLine(double beta, double r, double mu_m) {
  return r + (mu_m - r) * beta;
}

}  // namespace m4f::ch05

#endif  // M4F_CH05_PORTFOLIO_H_
