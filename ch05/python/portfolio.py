"""Sections 5.3-5.4: several securities and the Capital Asset Pricing Model.

Weights w, expected returns m and the covariance matrix C are plain lists;
u = (1, ..., 1). The formulas are the book's matrix forms:
    mu_V = w m^T, sigma_V^2 = w C w^T.
"""
from __future__ import annotations

import math
from collections.abc import Sequence

Vec = list[float]
Mat = list[list[float]]


# Linear algebra kept deliberately plain so all three languages compute alike.
def dot(a: Sequence[float], b: Sequence[float]) -> float:
    s = 0.0
    for x, y in zip(a, b):
        s += x * y
    return s


def vec_mat(v: Sequence[float], M: Mat) -> Vec:
    """v M, for a row vector v."""
    return [dot(v, [M[i][j] for i in range(len(M))]) for j in range(len(M[0]))]


def inverse(M: Mat) -> Mat:
    """Gauss-Jordan elimination with partial pivoting."""
    n = len(M)
    a = [list(row) + [1.0 if i == j else 0.0 for j in range(n)] for i, row in enumerate(M)]
    for c in range(n):
        piv = max(range(c, n), key=lambda r: abs(a[r][c]))
        a[c], a[piv] = a[piv], a[c]
        d = a[c][c]
        a[c] = [x / d for x in a[c]]
        for r in range(n):
            if r != c:
                k = a[r][c]
                a[r] = [x - k * y for x, y in zip(a[r], a[c])]
    return [row[n:] for row in a]


def covariance_matrix(sigma: Sequence[float], rho: Mat) -> Mat:
    """c_ij = rho_ij sigma_i sigma_j"""
    n = len(sigma)
    return [[rho[i][j] * sigma[i] * sigma[j] for j in range(n)] for i in range(n)]


# 5.3.1 Risk and expected return on a portfolio
def portfolio_mean(w: Sequence[float], m: Sequence[float]) -> float:
    return dot(w, m)


def portfolio_std(w: Sequence[float], C: Mat) -> float:
    return math.sqrt(dot(vec_mat(w, C), w))


# 5.3.2 Efficient frontier
def min_variance_portfolio(C: Mat) -> Vec:
    """w = u C^-1 / (u C^-1 u^T)"""
    uCi = vec_mat([1.0] * len(C), inverse(C))
    s = sum(uCi)
    return [x / s for x in uCi]


def min_variance_line(m: Sequence[float], C: Mat, mu_V: float) -> Vec:
    """Minimum variance portfolio with expected return mu_V. With
    a = u C^-1 u^T, b = u C^-1 m^T, c = m C^-1 m^T:
        w = ((c - b mu_V) u C^-1 + (a mu_V - b) m C^-1) / (a c - b^2)."""
    Ci = inverse(C)
    uCi = vec_mat([1.0] * len(C), Ci)
    mCi = vec_mat(m, Ci)
    a, b, c = sum(uCi), dot(uCi, m), dot(mCi, m)
    det = a * c - b * b
    return [((c - b * mu_V) * x + (a * mu_V - b) * y) / det for x, y in zip(uCi, mCi)]


# 5.4 Capital Asset Pricing Model, risk-free return R
def market_portfolio(m: Sequence[float], C: Mat, R: float) -> Vec:
    """Tangency portfolio: w_M = (m - R u) C^-1 / ((m - R u) C^-1 u^T)."""
    excess = vec_mat([x - R for x in m], inverse(C))
    s = sum(excess)
    return [x / s for x in excess]


def capital_market_line(sigma_V: float, R: float, mu_M: float, sigma_M: float) -> float:
    """5.4.1 mu_V = R + (mu_M - R) / sigma_M * sigma_V"""
    return R + (mu_M - R) / sigma_M * sigma_V


def beta(w: Sequence[float], w_M: Sequence[float], C: Mat) -> float:
    """5.4.2 beta_V = Cov(K_V, K_M) / sigma_M^2 = w C w_M^T / (w_M C w_M^T)."""
    CwM = vec_mat(w_M, C)  # C is symmetric, so w_M C = (C w_M^T)^T
    return dot(CwM, w) / dot(CwM, w_M)


def security_market_line(b: float, R: float, mu_M: float) -> float:
    """5.4.3 mu_V = R + (mu_M - R) beta_V"""
    return R + (mu_M - R) * b
