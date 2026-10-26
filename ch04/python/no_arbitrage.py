"""Sections 4.1.2-4.1.4: no arbitrage, the binomial tree and the Fundamental
Theorem of Asset Pricing.

A one-step market has one stock whose return over the step is one of the
scenario returns K_i, and a money market returning r. It admits no arbitrage
exactly when a risk-neutral probability exists: q_i > 0 with sum q_i K_i = r,
which happens exactly when min K_i < r < max K_i.
"""
from __future__ import annotations

from collections.abc import Callable, Sequence

from strategies import self_financing


def is_arbitrage_free(returns: Sequence[float], r: float) -> bool:
    return min(returns) < r < max(returns)


def arbitrage_strategy(returns: Sequence[float], r: float, S0: float,
                       A0: float) -> tuple[float, float] | None:
    """(x, y) with V(0) = x S0 + y A0 = 0 and V(1) >= 0 in every scenario,
    > 0 in at least one; None when the market is arbitrage-free."""
    if r >= max(returns):
        return -1.0, S0 / A0
    if r <= min(returns):
        return 1.0, -S0 / A0
    return None


def one_step_values(x: float, y: float, returns: Sequence[float], r: float, S0: float,
                    A0: float) -> list[float]:
    """V(1) = x S(1) + y A(1) in each scenario."""
    return [x * S0 * (1 + K) + y * A0 * (1 + r) for K in returns]


def risk_neutral_probs(returns: Sequence[float], r: float) -> list[float] | None:
    """A strictly positive q with sum q_i K_i = r, or None when there is none.
    With two scenarios it is the unique p*; with more, one of many: a mix of the
    uniform distribution (weight w) and a two-point law on the lowest and highest
    return."""
    if not is_arbitrage_free(returns, r):
        return None
    n = len(returns)
    hi, lo = max(returns), min(returns)
    ih, il = returns.index(hi), returns.index(lo)
    mean = sum(returns) / n
    w = 1.0
    if mean > lo:
        w = min(w, (r - lo) / (mean - lo))
    if hi > mean:
        w = min(w, (hi - r) / (hi - mean))
    w = 0.5 * w if n > 2 else 0.0
    target = (r - w * mean) / (1 - w)
    p = (target - lo) / (hi - lo)
    q = [w / n] * n
    q[ih] += (1 - w) * p
    q[il] += (1 - w) * (1 - p)
    return q


# 4.1.3 Binomial tree: a strategy is a rule x(n, moves) giving the shares held
# over (n-1, n] from the moves made so far (1 = up, 0 = down; len(moves) = n-1).
Rule = Callable[[int, Sequence[int]], float]


def expected_discounted_value(S0: float, u: float, d: float, r: float, N: int, V0: float,
                              rule: Rule, p: float) -> float:
    """E(V~(N)) over all 2^N paths, with V~(N) = V(N) / (1+r)^N and A(0) = 1.
    Under p = p* this equals V(0) for every self-financing rule."""
    A = [(1 + r) ** n for n in range(N + 1)]
    total = 0.0
    for mask in range(1 << N):
        moves = [(mask >> i) & 1 for i in range(N)]
        S = [S0]
        for m in moves:
            S.append(S[-1] * (1 + (u if m else d)))
        x = [rule(n, moves[:n - 1]) for n in range(1, N + 1)]
        _, V = self_financing(V0, S, A, x)
        ups = sum(moves)
        total += p ** ups * (1 - p) ** (N - ups) * V[N] / A[N]
    return total
