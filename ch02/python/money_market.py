"""Section 2.2 of Chapter 2: the money market.

Zero-coupon bonds, coupon bonds and the money market account. Bond prices are
per unit face value unless a face value is given.

Book notation: T is the maturity (`maturity`), B a unit bond price
(`bond_price`), F the face value (`face`), C the coupon (`coupon`), y the
yield per period, A(0) the initial balance (`a0`).
"""

from __future__ import annotations

import math


# 2.2.1 Zero-coupon bonds. B(t, T) is the time-t price of a bond paying 1 at T.
def zero_coupon_price(r: float, maturity: float, m: int | None = None) -> float:
    """Price at 0 of a unit zero-coupon bond maturing at `maturity`.

    Args:
        r: The implied interest rate.
        maturity: Time to maturity T in years.
        m: Compounding periods per year; None means continuous compounding.

    Returns:
        The bond price B(0, T).
    """
    if m is None:
        return math.exp(-r * maturity)
    return (1 + r / m) ** (-maturity * m)


def implied_rate(
    bond_price: float, maturity: float, m: int | None = None
) -> float:
    """Rate implied by a unit bond price (inverse of zero_coupon_price).

    Args:
        bond_price: Price B of a unit zero-coupon bond.
        maturity: Time to maturity T in years.
        m: Compounding periods per year; None means continuous compounding.

    Returns:
        The rate r with zero_coupon_price(r, maturity, m) == bond_price.
    """
    if m is None:
        return -math.log(bond_price) / maturity
    return m * (bond_price ** (-1 / (maturity * m)) - 1)


# 2.2.2 Coupon bonds: n coupons C at the end of each period, face value F paid
# with the last coupon; y is the yield per period.
def coupon_bond_price(face: float, coupon: float, n: int, y: float) -> float:
    price = face * (1 + y) ** -n
    for k in range(1, n + 1):
        price += coupon * (1 + y) ** -k
    return price


def yield_to_maturity(
    price: float,
    face: float,
    coupon: float,
    n: int,
    lo: float = -0.99,
    hi: float = 10.0,
    tol: float = 1e-12,
) -> float:
    """Per-period yield y with coupon_bond_price(face, coupon, n, y) == price.

    Uses bisection on [lo, hi]; the price is strictly decreasing in y.
    """
    for _ in range(200):
        mid = (lo + hi) / 2
        if coupon_bond_price(face, coupon, n, mid) > price:
            lo = mid
        else:
            hi = mid
        if hi - lo < tol:
            break
    return (lo + hi) / 2


# 2.2.3 Money market account: A(t) = A(0) e^(r t) at a constant continuous
# rate.
def money_market_value(a0: float, r: float, t: float) -> float:
    return a0 * math.exp(r * t)


def money_market_rollover(a0: float, bond_prices: list[float]) -> float:
    """Money market account built by rolling over short zero-coupon bonds.

    At each step the whole balance buys bonds at unit price B, which pay 1 per
    bond.
    """
    balance = a0
    for bond_price in bond_prices:
        balance /= bond_price
    return balance
