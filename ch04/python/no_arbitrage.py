"""Sections 4.1.2-4.1.4: no arbitrage, the binomial tree and the FTAP.

A one-step market has one stock whose return over the step is one of the
scenario returns K_i, and a money market returning r. It admits no arbitrage
exactly when a risk-neutral probability exists: q_i > 0 with
sum q_i K_i = r, which happens exactly when min K_i < r < max K_i.

Names follow the book, lower-cased: s0 = S(0) and a0 = A(0) are the stock
and money market prices at time 0, v0 = V(0) is the initial wealth, u and d
are the up and down returns of the binomial tree, p is the probability of an
up move, and x, y are the numbers of shares and money market units held.
"""

from __future__ import annotations

from collections.abc import Callable, Sequence


def is_arbitrage_free(returns: Sequence[float], r: float) -> bool:
    return min(returns) < r < max(returns)


def arbitrage_strategy(
    returns: Sequence[float], r: float, s0: float, a0: float
) -> tuple[float, float] | None:
    """Builds an arbitrage if there is one.

    Returns:
        (x, y) with V(0) = x S(0) + y A(0) = 0 and V(1) >= 0 in every
        scenario, > 0 in at least one; None when the market is
        arbitrage-free.
    """
    if r >= max(returns):
        return -1.0, s0 / a0
    if r <= min(returns):
        return 1.0, -s0 / a0
    return None


def one_step_values(
    x: float,
    y: float,
    returns: Sequence[float],
    r: float,
    s0: float,
    a0: float,
) -> list[float]:
    """V(1) = x S(1) + y A(1) in each scenario."""
    return [x * s0 * (1 + k) + y * a0 * (1 + r) for k in returns]


def risk_neutral_probs(
    returns: Sequence[float], r: float
) -> list[float] | None:
    """A strictly positive q with sum q_i K_i = r, or None when there is none.

    With two scenarios it is the unique p*; with more, one of many: a mix of
    the uniform distribution (weight w) and a two-point law on the lowest and
    highest return.
    """
    if not is_arbitrage_free(returns, r):
        return None
    n = len(returns)
    hi, lo = max(returns), min(returns)
    i_hi, i_lo = returns.index(hi), returns.index(lo)
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
    q[i_hi] += (1 - w) * p
    q[i_lo] += (1 - w) * (1 - p)
    return q


# 4.1.3 Binomial tree: a strategy is a rule x(n, moves) giving the shares held
# over (n-1, n] from the moves made so far (1 = up, 0 = down;
# len(moves) = n-1).
Rule = Callable[[int, Sequence[int]], float]


def expected_discounted_value(
    s0: float,
    u: float,
    d: float,
    r: float,
    num_steps: int,
    v0: float,
    rule: Rule,
    p: float,
) -> float:
    """E(V~(N)) over all 2^N paths, with V~(N) = V(N) / (1+r)^N, A(0) = 1.

    Under p = p* this equals V(0) for every self-financing rule.

    The paths are walked depth first, so paths share their common prefix: the
    rule is called once per tree node (2^N - 1 calls in all, with the moves so
    far), and nothing is built per path.
    """
    a = [(1 + r) ** n for n in range(num_steps + 1)]
    moves: list[int] = []
    total = 0.0

    def walk(n: int, s: float, v: float, weight: float) -> None:
        # At time n-1 with stock price s and strategy value v, reached with
        # probability `weight`; chooses x(n) and branches on the n-th move.
        nonlocal total
        if n > num_steps:
            total += weight * v / a[num_steps]
            return
        x = rule(n, moves)
        y = (v - x * s) / a[n - 1]
        for move in (0, 1):
            s_next = s * (1 + (u if move else d))
            moves.append(move)
            walk(
                n + 1,
                s_next,
                y * a[n] + x * s_next,
                weight * (p if move else 1 - p),
            )
            moves.pop()

    walk(1, s0, v0, 1.0)
    return total
