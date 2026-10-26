"""Section 10.2: general term structure. The market gives zero-coupon bond
prices B(0,T) for a grid of maturities T_1 < ... < T_n; all rates are
continuously compounded. In a deterministic term structure the implied
forward rates are the future short rates, and B(t,T) = B(0,T)/B(0,t).
"""
from __future__ import annotations

import math


def plain_sum(xs) -> float:
    """Left-to-right float sum (builtin sum() is compensated since Python 3.12
    and would not match the C++ and Kotlin loops bit for bit)."""
    t = 0.0
    for x in xs:
        t += x
    return t


def yield_rate(B: float, T: float) -> float:
    """y(0,T) = -ln B(0,T) / T"""
    return -math.log(B) / T


def forward_rate(B1: float, T1: float, B2: float, T2: float) -> float:
    """f(0,T1,T2) = (ln B(0,T1) - ln B(0,T2)) / (T2 - T1); B(0,0) = 1 gives y(0,T2)."""
    return (math.log(B1) - math.log(B2)) / (T2 - T1)


def forward_rates(maturities: list[float], prices: list[float]) -> list[float]:
    """f(0,T_{i-1},T_i) for each i, with T_0 = 0."""
    out = []
    Bp, Tp = 1.0, 0.0
    for T, B in zip(maturities, prices):
        out.append(forward_rate(Bp, Tp, B, T))
        Bp, Tp = B, T
    return out


def prices_from_forward_rates(maturities: list[float], f: list[float]) -> list[float]:
    """B(0,T_n) = exp(-sum_{i<=n} f(0,T_{i-1},T_i)(T_i - T_{i-1}))"""
    out = []
    acc, Tp = 0.0, 0.0
    for T, fi in zip(maturities, f):
        acc += fi * (T - Tp)
        out.append(math.exp(-acc))
        Tp = T
    return out


def implied_price(B_t: float, B_T: float) -> float:
    """B(t,T) implied by no arbitrage in a deterministic term structure: B(0,T)/B(0,t)."""
    return B_T / B_t


def coupon_bond_price(flows_t: list[float], flows_C: list[float], maturities: list[float],
                      prices: list[float]) -> float:
    """Price of a coupon bond = sum C_i B(0,t_i), with t_i on the grid."""
    total = 0.0
    for t, C in zip(flows_t, flows_C):
        total += C * prices[maturities.index(t)]
    return total


def yield_to_maturity(P: float, flows_t: list[float], flows_C: list[float]) -> float:
    """The single continuous yield y with sum C_i e^(-y t_i) = P (bisection)."""
    lo, hi = -1.0, 1.0
    for _ in range(200):
        mid = (lo + hi) / 2
        if plain_sum(C * math.exp(-mid * t) for t, C in zip(flows_t, flows_C)) > P:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2


def money_market_account(maturities: list[float], prices: list[float]) -> list[float]:
    """10.2.2: A(T_n) from rolling over one-period bonds at the (implied, hence
    realised) short rates: A(T_n) = prod 1/B(T_{i-1},T_i) = 1/B(0,T_n)."""
    out = []
    A = 1.0
    Bp = 1.0
    for B in prices:
        A = A / implied_price(Bp, B)
        out.append(A)
        Bp = B
    return out
