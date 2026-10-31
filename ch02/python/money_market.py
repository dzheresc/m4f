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
    """Price F v + C (1 - v) / y with v = (1+y)^-n.

    The coupons form a geometric series; 1 - v is computed as
    -expm1(-n log1p(y)) to stay accurate for small y.
    """
    if y == 0:
        return face + coupon * n
    log_v = -n * math.log1p(y)
    return face * math.exp(log_v) - coupon * math.expm1(log_v) / y


def coupon_bond_price_slope(
    face: float, coupon: float, n: int, y: float
) -> float:
    """Derivative of coupon_bond_price in y.

    -(n F v + C sum_k k (1+y)^-k) / (1+y).
    """
    discount = 1 / (1 + y)
    factor = 1.0
    weighted = 0.0
    for k in range(1, n + 1):
        factor *= discount
        weighted += k * factor
    return -(n * face * factor + coupon * weighted) * discount


def yield_to_maturity(
    price: float,
    face: float,
    coupon: float,
    n: int,
    lo: float = -0.99,
    hi: float = 10.0,
    tol: float = 1e-12,
) -> float:
    """Per-period yield y in [lo, hi] with coupon_bond_price(...) == price.

    Newton's method, kept inside a shrinking bisection bracket (the price is
    strictly decreasing in y), so it falls back to bisection whenever a Newton
    step would leave the bracket and returns the nearer end when the price is
    out of range. Stops once the bracket or the Newton step is narrower than
    `tol`. `n` is the number of coupon periods.
    """
    # Start from the textbook approximation (C + (F - P) / n) / P.
    y = (coupon + (face - price) / n) / price
    if not lo < y < hi:
        y = (lo + hi) / 2
    for _ in range(200):
        if hi - lo < tol:
            break
        excess = coupon_bond_price(face, coupon, n, y) - price
        if excess == 0:
            return y
        if excess > 0:
            lo = y
        else:
            hi = y
        nxt = y - excess / coupon_bond_price_slope(face, coupon, n, y)
        if not lo < nxt < hi:
            nxt = (lo + hi) / 2
        if abs(nxt - y) < tol:
            return nxt
        y = nxt
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
