"""Section 2.2, money market: zero-coupon bonds, coupon bonds and the money
market account. Bond prices are per unit face value unless F is given.
"""
from __future__ import annotations

import math


# 2.2.1 Zero-coupon bonds. B(t, T) is the time-t price of a bond paying 1 at T.
def zero_coupon_price(r: float, T: float, m: int | None = None) -> float:
    """Price at 0 of a unit zero-coupon bond maturing at T, for an implied rate r
    compounded m times a year (None: continuously)."""
    return math.exp(-r * T) if m is None else (1 + r / m) ** (-T * m)


def implied_rate(B: float, T: float, m: int | None = None) -> float:
    """Rate r implied by a unit bond price B with maturity T (inverse of
    zero_coupon_price); m=None means continuous compounding."""
    return -math.log(B) / T if m is None else m * (B ** (-1 / (T * m)) - 1)


# 2.2.2 Coupon bonds: n coupons C at the end of each period, face value F paid
# with the last coupon; y is the yield per period.
def coupon_bond_price(F: float, C: float, n: int, y: float) -> float:
    price = F * (1 + y) ** -n
    for k in range(1, n + 1):
        price += C * (1 + y) ** -k
    return price


def yield_to_maturity(price: float, F: float, C: float, n: int,
                      lo: float = -0.99, hi: float = 10.0, tol: float = 1e-12) -> float:
    """Per-period yield y with coupon_bond_price(F, C, n, y) = price (bisection;
    the price is strictly decreasing in y)."""
    for _ in range(200):
        mid = (lo + hi) / 2
        if coupon_bond_price(F, C, n, mid) > price:
            lo = mid
        else:
            hi = mid
        if hi - lo < tol:
            break
    return (lo + hi) / 2


# 2.2.3 Money market account: A(t) = A(0) e^(r t) at a constant continuous rate.
def money_market_value(A0: float, r: float, t: float) -> float:
    return A0 * math.exp(r * t)



def money_market_rollover(A0: float, prices: list[float]) -> float:
    """Money market account built by rolling over short zero-coupon bonds: at each
    step the whole balance buys bonds at unit price B, which pay 1 per bond."""
    A = A0
    for B in prices:
        A /= B
    return A
