"""Section 6.2, futures.

Pricing in the binomial model, marking to market, and hedging with futures.
"""

from collections.abc import Sequence
import math


# Section 6.2.1: pricing. In the binomial tree (s0, u, d, r per step) the
# futures price for delivery at N is f(n) = E*(S(N) | S(n)): f(N) = S(N) and
# backwards f(n, k) = p* f(n+1, k+1) + (1 - p*) f(n+1, k).
def futures_tree(
    s0: float, u: float, d: float, r: float, num_steps: int
) -> list[list[float]]:
    """Returns f[n][k] after k up moves out of n."""
    q = (r - d) / (u - d)
    f = [
        [
            s0 * (1 + u) ** k * (1 + d) ** (num_steps - k)
            for k in range(num_steps + 1)
        ]
    ]
    for n in range(num_steps - 1, -1, -1):
        later = f[0]
        f.insert(
            0, [q * later[k + 1] + (1 - q) * later[k] for k in range(n + 1)]
        )
    return f


def margin_account(
    prices: Sequence[float], size: float, initial: float, maintenance: float
) -> list[tuple[float, float]]:
    """Marking to market for one long contract on `size` units.

    Each day the account gains size * (f(n) - f(n-1)); when it falls below the
    maintenance margin, a call restores it to the initial margin.

    Returns:
        (balance after settlement, margin call) per day.
    """
    balance = initial
    out: list[tuple[float, float]] = []
    for n in range(1, len(prices)):
        balance += size * (prices[n] - prices[n - 1])
        call = initial - balance if balance < maintenance else 0.0
        out.append((balance, call))
        balance += call
    return out


# Section 6.2.2: hedging with futures. Selling h futures per unit of spot
# exposure turns a spot change dS into dS - h df. In scenario i, with
# probability p[i], the spot price changes by ds[i] and the futures price by
# df[i].
def min_variance_hedge_ratio(
    ds: Sequence[float], df: Sequence[float], p: Sequence[float]
) -> float:
    """h* = Cov(dS, df) / Var(df)."""
    mean_s = mean_f = 0.0
    for a, b, p_i in zip(ds, df, p):
        mean_s += p_i * a
        mean_f += p_i * b
    cov = var = 0.0
    for a, b, p_i in zip(ds, df, p):
        cov += p_i * (a - mean_s) * (b - mean_f)
        var += p_i * (b - mean_f) * (b - mean_f)
    return cov / var


def hedged_std(
    h: float, ds: Sequence[float], df: Sequence[float], p: Sequence[float]
) -> float:
    """Standard deviation of dS - h df."""
    x = [a - h * b for a, b in zip(ds, df)]
    m = 0.0
    for a, p_i in zip(x, p):
        m += p_i * a
    v = 0.0
    for a, p_i in zip(x, p):
        v += p_i * (a - m) * (a - m)
    return math.sqrt(v)
