"""Sections 5.3-5.4: several securities and the Capital Asset Pricing Model.

Weights w, expected returns m and the covariance matrix c are plain lists;
u = (1, ..., 1). The formulas are the book's matrix forms:
    mu_V = w m^T, sigma_V^2 = w C w^T.
"""

from collections.abc import Sequence
import math

Vec = list[float]
Mat = list[list[float]]


# Linear algebra kept deliberately plain so all three languages compute alike.
def dot(a: Sequence[float], b: Sequence[float]) -> float:
    """Inner product of a and b."""
    s = 0.0
    for x, y in zip(a, b):
        s += x * y
    return s


def vec_mat(v: Sequence[float], m: Mat) -> Vec:
    """v M, for a row vector v."""
    return [dot(v, [m[i][j] for i in range(len(m))]) for j in range(len(m[0]))]


def inverse(m: Mat) -> Mat:
    """Gauss-Jordan elimination with partial pivoting."""
    n = len(m)
    a = [
        list(row) + [1.0 if i == j else 0.0 for j in range(n)]
        for i, row in enumerate(m)
    ]
    for c in range(n):
        pivot = max(range(c, n), key=lambda r, c=c: abs(a[r][c]))
        a[c], a[pivot] = a[pivot], a[c]
        d = a[c][c]
        a[c] = [x / d for x in a[c]]
        for r in range(n):
            if r != c:
                k = a[r][c]
                a[r] = [x - k * y for x, y in zip(a[r], a[c])]
    return [row[n:] for row in a]


def covariance_matrix(sigma: Sequence[float], rho: Mat) -> Mat:
    """c_ij = rho_ij sigma_i sigma_j."""
    n = len(sigma)
    return [
        [rho[i][j] * sigma[i] * sigma[j] for j in range(n)] for i in range(n)
    ]


# Section 5.3.1: risk and expected return on a portfolio.
def portfolio_mean(w: Sequence[float], m: Sequence[float]) -> float:
    """mu_V = w m^T."""
    return dot(w, m)


def portfolio_std(w: Sequence[float], cov: Mat) -> float:
    """sigma_V = sqrt(w C w^T)."""
    return math.sqrt(dot(vec_mat(w, cov), w))


# Section 5.3.2: efficient frontier.
def min_variance_portfolio(cov: Mat) -> Vec:
    """w = u C^-1 / (u C^-1 u^T)."""
    u_cov_inv = vec_mat([1.0] * len(cov), inverse(cov))
    s = sum(u_cov_inv)
    return [x / s for x in u_cov_inv]


def min_variance_line(m: Sequence[float], cov: Mat, mu_v: float) -> Vec:
    """Minimum variance portfolio with expected return mu_v.

    With a = u C^-1 u^T, b = u C^-1 m^T, c = m C^-1 m^T:
        w = ((c - b mu_v) u C^-1 + (a mu_v - b) m C^-1) / (a c - b^2).
    """
    cov_inv = inverse(cov)
    u_cov_inv = vec_mat([1.0] * len(cov), cov_inv)
    m_cov_inv = vec_mat(m, cov_inv)
    a, b, c = sum(u_cov_inv), dot(u_cov_inv, m), dot(m_cov_inv, m)
    det = a * c - b * b
    return [
        ((c - b * mu_v) * x + (a * mu_v - b) * y) / det
        for x, y in zip(u_cov_inv, m_cov_inv)
    ]


# Section 5.4: Capital Asset Pricing Model, risk-free return r.
def market_portfolio(m: Sequence[float], cov: Mat, r: float) -> Vec:
    """Tangency portfolio: w_M = (m - r u) C^-1 / ((m - r u) C^-1 u^T)."""
    excess = vec_mat([x - r for x in m], inverse(cov))
    s = sum(excess)
    return [x / s for x in excess]


def capital_market_line(
    sigma_v: float, r: float, mu_m: float, sigma_m: float
) -> float:
    """Section 5.4.1: mu_V = r + (mu_M - r) / sigma_M * sigma_V."""
    return r + (mu_m - r) / sigma_m * sigma_v


def beta(w: Sequence[float], w_market: Sequence[float], cov: Mat) -> float:
    """Section 5.4.2: beta_V = Cov(K_V, K_M) / sigma_M^2.

    Computed as w C w_M^T / (w_M C w_M^T).
    """
    # C is symmetric, so w_M C = (C w_M^T)^T.
    cov_w_market = vec_mat(w_market, cov)
    return dot(cov_w_market, w) / dot(cov_w_market, w_market)


def security_market_line(beta_v: float, r: float, mu_m: float) -> float:
    """Section 5.4.3: mu_V = r + (mu_M - r) beta_V."""
    return r + (mu_m - r) * beta_v
