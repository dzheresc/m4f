"""Section 11.3: interest rate derivatives on the binomial tree.

Floating payments are the simple one-period rate fixed at (n,k) and paid at
n+1: L(n,k) = 1/B(n,n+1;k) - 1 = e^(r(n,k) tau) - 1, on a notional of 1.
"""
from __future__ import annotations

from rate_tree import Lattice, bond_lattice, discount_back, one_step_price


def plain_sum(xs) -> float:
    """Left-to-right float sum (builtin sum() is compensated since Python 3.12
    and would not match the C++ and Kotlin loops bit for bit)."""
    t = 0.0
    for x in xs:
        t += x
    return t


def floating_rate(rates: Lattice, tau: float, n: int, k: int) -> float:
    return 1 / one_step_price(rates[n][k], tau) - 1


# 11.3.1 Options on a zero-coupon bond maturing at N, expiring at n_exp <= N.
def bond_option(rates: Lattice, probs: Lattice, tau: float, N: int, n_exp: int, X: float, is_call: bool,
                american: bool = False) -> Lattice:
    B = bond_lattice(rates, probs, tau, N)
    payoff = (lambda b: max(b - X, 0.0)) if is_call else (lambda b: max(X - b, 0.0))
    terminal = [payoff(B[n_exp][k]) for k in range(n_exp + 1)]
    early = (lambda n, k: payoff(B[n][k])) if american else None
    return discount_back(rates, probs, tau, terminal, early=early)


# 11.3.2 Swaps: fixed K against floating L(n,k), N payments at 1..N on notional 1.
def swap_rate(B0: list[float]) -> float:
    """The fixed rate making the swap worthless: K sum B(0,n) = 1 - B(0,N)."""
    return (1 - B0[-1]) / plain_sum(B0)


def price_payments(rates: Lattice, probs: Lattice, tau: float, N: int, payment) -> Lattice:
    """Value lattice of a stream paying payment(n,k) at n+1 for n = 0..N-1."""
    return discount_back(rates, probs, tau, [0.0] * (N + 1), payment=payment)


def payer_swap(rates: Lattice, probs: Lattice, tau: float, N: int, K: float) -> Lattice:
    """Receive floating, pay fixed K."""
    return price_payments(rates, probs, tau, N, lambda n, k: floating_rate(rates, tau, n, k) - K)


# 11.3.3 Caps and floors: caplet pays max(L(n,k) - K, 0), floorlet max(K - L(n,k), 0).
def cap(rates: Lattice, probs: Lattice, tau: float, N: int, K: float) -> Lattice:
    return price_payments(rates, probs, tau, N, lambda n, k: max(floating_rate(rates, tau, n, k) - K, 0.0))


def floor(rates: Lattice, probs: Lattice, tau: float, N: int, K: float) -> Lattice:
    return price_payments(rates, probs, tau, N, lambda n, k: max(K - floating_rate(rates, tau, n, k), 0.0))


def caplet(rates: Lattice, probs: Lattice, tau: float, n_fix: int, K: float) -> float:
    """Value at 0 of the single caplet fixed at n_fix and paid at n_fix + 1."""
    return price_payments(rates, probs, tau, n_fix + 1,
                          lambda n, k: max(floating_rate(rates, tau, n, k) - K, 0.0) if n == n_fix else 0.0)[0][0]
