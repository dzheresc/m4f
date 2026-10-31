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
    """v M, for a row vector v.

    Row by row, so no column is copied; each out[j] still sums v[i] m[i][j] in
    order of i.
    """
    out = [0.0] * len(m[0])
    for v_i, row in zip(v, m):
        out = [o + v_i * x for o, x in zip(out, row)]
    return out


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
        # Columns left of c are already unit columns (exact zeros off the
        # diagonal), so each row operation starts at column c.
        d = a[c][c]
        pivot_row = a[c][:c] + [x / d for x in a[c][c:]]
        a[c] = pivot_row
        tail = pivot_row[c:]
        for r in range(n):
            k = a[r][c]
            if r != c and k != 0:
                row = a[r]
                row[c:] = [x - k * y for x, y in zip(row[c:], tail)]
    return [row[n:] for row in a]


def solve_symmetric(c: Mat, rhs: list[Sequence[float]]) -> list[Vec]:
    """Solves C x = b for each right-hand side b, for a symmetric C.

    Uses a Cholesky factorisation C = L L^T (n^3/6 multiply-adds, then n^2 per
    right-hand side) when C is positive definite, as a covariance matrix of
    non-redundant assets is; otherwise falls back to the explicit inverse. For
    symmetric C, x = b C^-1 read as a row vector, so this replaces
    vec_mat(b, inverse(c)) without forming the inverse.
    """
    n = len(c)
    lower: list[list[float]] = []  # Row i holds L[i][0..i].
    for i in range(n):
        row_i: list[float] = []
        for j in range(i + 1):
            row_j = lower[j] if j < i else row_i
            total = c[i][j]
            for l_ik, l_jk in zip(row_i, row_j):  # k < j.
                total -= l_ik * l_jk
            if i == j:
                if not total > 0:
                    inv = inverse(c)
                    return [vec_mat(b, inv) for b in rhs]
                row_i.append(math.sqrt(total))
            else:
                row_i.append(total / row_j[j])
        lower.append(row_i)
    solved = []
    for b in rhs:
        x = list(b)
        for i in range(n):  # L z = b.
            total = x[i]
            for l_ik, x_k in zip(lower[i], x[:i]):
                total -= l_ik * x_k
            x[i] = total / lower[i][i]
        for i in range(n - 1, -1, -1):  # L^T x = z.
            total = x[i]
            for k in range(i + 1, n):
                total -= lower[k][i] * x[k]
            x[i] = total / lower[i][i]
        solved.append(x)
    return solved


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
    (u_cov_inv,) = solve_symmetric(cov, [[1.0] * len(cov)])
    s = sum(u_cov_inv)
    return [x / s for x in u_cov_inv]


def min_variance_line(m: Sequence[float], cov: Mat, mu_v: float) -> Vec:
    """Minimum variance portfolio with expected return mu_v.

    With a = u C^-1 u^T, b = u C^-1 m^T, c = m C^-1 m^T:
        w = ((c - b mu_v) u C^-1 + (a mu_v - b) m C^-1) / (a c - b^2).
    """
    u_cov_inv, m_cov_inv = solve_symmetric(cov, [[1.0] * len(cov), m])
    a, b, c = sum(u_cov_inv), dot(u_cov_inv, m), dot(m_cov_inv, m)
    det = a * c - b * b
    return [
        ((c - b * mu_v) * x + (a * mu_v - b) * y) / det
        for x, y in zip(u_cov_inv, m_cov_inv)
    ]


# Section 5.4: Capital Asset Pricing Model, risk-free return r.
def market_portfolio(m: Sequence[float], cov: Mat, r: float) -> Vec:
    """Tangency portfolio: w_M = (m - r u) C^-1 / ((m - r u) C^-1 u^T)."""
    (excess,) = solve_symmetric(cov, [[x - r for x in m]])
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
