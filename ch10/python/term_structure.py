"""Section 10.2: general term structure.

The market gives zero-coupon bond prices B(0,T) for a grid of maturities
T_1 < ... < T_n; all rates are continuously compounded. In a deterministic
term structure the implied forward rates are the future short rates, and
B(t,T) = B(0,T)/B(0,t).
"""

from collections.abc import Iterable
import math


def plain_sum(xs: Iterable[float]) -> float:
    """Left-to-right float sum.

    The builtin sum() is compensated since Python 3.12 and would not match the
    C++ and Kotlin loops bit for bit.
    """
    t = 0.0
    for x in xs:
        t += x
    return t


def yield_rate(bond_price: float, maturity: float) -> float:
    """y(0,T) = -ln B(0,T) / T."""
    return -math.log(bond_price) / maturity


def forward_rate(b1: float, t1: float, b2: float, t2: float) -> float:
    """f(0,T1,T2) = (ln B(0,T1) - ln B(0,T2)) / (T2 - T1).

    B(0,0) = 1 gives y(0,T2).
    """
    return (math.log(b1) - math.log(b2)) / (t2 - t1)


def forward_rates(maturities: list[float], prices: list[float]) -> list[float]:
    """f(0,T_{i-1},T_i) for each i, with T_0 = 0."""
    out = []
    b_prev, t_prev = 1.0, 0.0
    for t, b in zip(maturities, prices):
        out.append(forward_rate(b_prev, t_prev, b, t))
        b_prev, t_prev = b, t
    return out


def prices_from_forward_rates(
    maturities: list[float], f: list[float]
) -> list[float]:
    """B(0,T_n) = exp(-sum_{i<=n} f(0,T_{i-1},T_i)(T_i - T_{i-1}))."""
    out = []
    acc, t_prev = 0.0, 0.0
    for t, f_i in zip(maturities, f):
        acc += f_i * (t - t_prev)
        out.append(math.exp(-acc))
        t_prev = t
    return out


def implied_price(b_t: float, b_maturity: float) -> float:
    """B(t,T) = B(0,T)/B(0,t), by no arbitrage in a deterministic structure."""
    return b_maturity / b_t


def coupon_bond_price(
    flow_times: list[float],
    flow_amounts: list[float],
    maturities: list[float],
    prices: list[float],
) -> float:
    """Price of a coupon bond = sum C_i B(0,t_i), with t_i on the grid."""
    total = 0.0
    for t, amount in zip(flow_times, flow_amounts):
        total += amount * prices[maturities.index(t)]
    return total


def yield_to_maturity(
    bond_price: float, flow_times: list[float], flow_amounts: list[float]
) -> float:
    """The single continuous yield y with sum C_i e^(-y t_i) = P (bisection)."""
    lo, hi = -1.0, 1.0
    for _ in range(200):
        mid = (lo + hi) / 2
        value = plain_sum(
            amount * math.exp(-mid * t)
            for t, amount in zip(flow_times, flow_amounts)
        )
        if value > bond_price:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2


def money_market_account(
    maturities: list[float], prices: list[float]  # pylint: disable=unused-argument
) -> list[float]:
    """Section 10.2.2: rolling over one-period bonds at the short rates.

    The short rates are implied, hence realised:
    A(T_n) = prod 1/B(T_{i-1},T_i) = 1/B(0,T_n).
    """
    out = []
    a = 1.0
    b_prev = 1.0
    for b in prices:
        a = a / implied_price(b_prev, b)
        out.append(a)
        b_prev = b
    return out
