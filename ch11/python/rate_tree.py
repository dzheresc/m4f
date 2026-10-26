"""Sections 11.1-11.2: the binomial tree model of the short rate.

Time steps of length tau; node (n,k) is reached after n steps of which k were
"up". A Lattice holds one value per node: L[n][k], k = 0..n. The short rate
r(n,k) applies over [n, n+1], so the one-step bond is B(n,n+1;k) = e^(-r(n,k) tau)
and the money market account grows by e^(r tau) each step. Risk-neutral
probabilities p*(n,k) of an up move may vary by node.
"""
from __future__ import annotations

import math

Lattice = list[list[float]]


def rate_lattice(r0: float, up: float, down: float, N: int) -> Lattice:
    """Additive tree: r(n,k) = r0 + k up + (n-k) down (down is negative for a fall)."""
    return [[r0 + k * up + (n - k) * down for k in range(n + 1)] for n in range(N + 1)]


def constant_probs(p: float, N: int) -> Lattice:
    return [[p for _ in range(n + 1)] for n in range(N)]


def one_step_price(r: float, tau: float) -> float:
    """B(n,n+1) = e^(-r tau)"""
    return math.exp(-r * tau)


def money_market(rates: Lattice, tau: float, path: str) -> list[float]:
    """A(0..len(path)) along a path of 'u'/'d' moves: A(n+1) = A(n) e^(r(n,k) tau)."""
    A = [1.0]
    k = 0
    for n, move in enumerate(path):
        A.append(A[-1] * math.exp(rates[n][k] * tau))
        if move == "u":
            k += 1
    return A


def discount_back(rates: Lattice, probs: Lattice, tau: float, terminal: list[float],
                  payment=None, early=None) -> Lattice:
    """Backward induction from level N = len(terminal)-1 down to 0:
    V(n,k) = e^(-r(n,k) tau) [ p* V(n+1,k+1) + (1-p*) V(n+1,k) + payment(n,k) ]
    with the payment (if any) made at n+1 but fixed at (n,k), and, if `early`
    is given, V(n,k) = max(V(n,k), early(n,k)) (American exercise)."""
    N = len(terminal) - 1
    V: Lattice = [[0.0] * (n + 1) for n in range(N + 1)]
    V[N] = list(terminal)
    for n in range(N - 1, -1, -1):
        for k in range(n + 1):
            p = probs[n][k]
            cont = p * V[n + 1][k + 1] + (1 - p) * V[n + 1][k]
            if payment is not None:
                cont += payment(n, k)
            v = one_step_price(rates[n][k], tau) * cont
            if early is not None:
                v = max(v, early(n, k))
            V[n][k] = v
    return V


def bond_lattice(rates: Lattice, probs: Lattice, tau: float, N: int) -> Lattice:
    """11.2: B(n,N;k) for all nodes, from B(N,N) = 1."""
    return discount_back(rates, probs, tau, [1.0] * (N + 1))


def bond_prices(rates: Lattice, probs: Lattice, tau: float, N: int) -> list[float]:
    """B(0,1), ..., B(0,N)"""
    return [bond_lattice(rates, probs, tau, m)[0][0] for m in range(1, N + 1)]


def yield_rate(B: float, n: int, N: int, tau: float) -> float:
    """y(n,N) = -ln B(n,N) / ((N-n) tau)"""
    return -math.log(B) / ((N - n) * tau)


def implied_prob(B: Lattice, rates: Lattice, tau: float, n: int, k: int) -> float:
    """11.2.1: the p*(n,k) that prices the bond with lattice B at node (n,k):
    B(n,N;k) e^(r tau) = p* B(n+1,N;k+1) + (1-p*) B(n+1,N;k)."""
    return (B[n][k] * math.exp(rates[n][k] * tau) - B[n + 1][k]) / (B[n + 1][k + 1] - B[n + 1][k])


def implied_probs(B: Lattice, rates: Lattice, tau: float) -> Lattice:
    N = len(B) - 1
    return [[implied_prob(B, rates, tau, n, k) for k in range(n + 1)] for n in range(N - 1)]
