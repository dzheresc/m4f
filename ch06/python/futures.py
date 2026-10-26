"""Section 6.2, futures: pricing in the binomial model, marking to market, and
hedging with futures.
"""
from __future__ import annotations

import math
from collections.abc import Sequence


# 6.2.1 Pricing. In the binomial tree (S0, u, d, r per step) the futures price
# for delivery at N is f(n) = E*(S(N) | S(n)): f(N) = S(N) and backwards
# f(n, k) = p* f(n+1, k+1) + (1 - p*) f(n+1, k).
def futures_tree(S0: float, u: float, d: float, r: float, N: int) -> list[list[float]]:
    """f[n][k] after k up moves out of n."""
    q = (r - d) / (u - d)
    f = [[S0 * (1 + u) ** k * (1 + d) ** (N - k) for k in range(N + 1)]]
    for n in range(N - 1, -1, -1):
        nxt = f[0]
        f.insert(0, [q * nxt[k + 1] + (1 - q) * nxt[k] for k in range(n + 1)])
    return f


def margin_account(prices: Sequence[float], size: float, initial: float,
                   maintenance: float) -> list[tuple[float, float]]:
    """Marking to market for one long contract on `size` units: each day the
    account gains size * (f(n) - f(n-1)); when it falls below the maintenance
    margin, a call restores it to the initial margin. Returns (balance after
    settlement, margin call) per day."""
    balance = initial
    out: list[tuple[float, float]] = []
    for n in range(1, len(prices)):
        balance += size * (prices[n] - prices[n - 1])
        call = initial - balance if balance < maintenance else 0.0
        out.append((balance, call))
        balance += call
    return out


# 6.2.2 Hedging with futures. Selling h futures per unit of spot exposure turns
# a spot change dS into dS - h df.
def min_variance_hedge_ratio(dS: Sequence[float], df: Sequence[float], p: Sequence[float]) -> float:
    """h* = Cov(dS, df) / Var(df)."""
    mS = mf = 0.0
    for a, b, q in zip(dS, df, p):
        mS += q * a
        mf += q * b
    cov = var = 0.0
    for a, b, q in zip(dS, df, p):
        cov += q * (a - mS) * (b - mf)
        var += q * (b - mf) * (b - mf)
    return cov / var


def hedged_std(h: float, dS: Sequence[float], df: Sequence[float], p: Sequence[float]) -> float:
    """Standard deviation of dS - h df."""
    x = [a - h * b for a, b in zip(dS, df)]
    m = 0.0
    for a, q in zip(x, p):
        m += q * a
    v = 0.0
    for a, q in zip(x, p):
        v += q * (a - m) * (a - m)
    return math.sqrt(v)
