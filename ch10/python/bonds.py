"""Section 10.1: maturity-independent yields. All bonds are priced with one
continuously compounded yield y, so B(t,T) = e^(-y(T-t)). A Bond is a list of
cash flows (t_i, C_i); coupons and the face value are flows.
"""
from __future__ import annotations

import math
from dataclasses import dataclass


def plain_sum(xs) -> float:
    """Left-to-right float sum (builtin sum() is compensated since Python 3.12
    and would not match the C++ and Kotlin loops bit for bit)."""
    t = 0.0
    for x in xs:
        t += x
    return t


@dataclass(frozen=True)
class Flow:
    t: float
    C: float


Bond = list[Flow]


def zero_coupon(face: float, T: float) -> Bond:
    return [Flow(T, face)]


def coupon_bond(face: float, coupon: float, n: int, dt: float = 1.0) -> Bond:
    """n coupons of `coupon` at dt, 2dt, ..., n dt; the face is repaid with the last."""
    return [Flow((i + 1) * dt, coupon + (face if i == n - 1 else 0.0)) for i in range(n)]


def price(bond: Bond, y: float, t: float = 0.0) -> float:
    """P(t) = sum over t_i > t of C_i e^(-y(t_i - t))"""
    return plain_sum(fl.C * math.exp(-y * (fl.t - t)) for fl in bond if fl.t > t)


def duration(bond: Bond, y: float, t: float = 0.0) -> float:
    """D = sum (t_i - t) C_i e^(-y(t_i - t)) / P; dP/dy = -D P"""
    return plain_sum((fl.t - t) * fl.C * math.exp(-y * (fl.t - t)) for fl in bond if fl.t > t) / price(bond, y, t)


def investment_value(bond: Bond, y: float, t: float) -> float:
    """10.1.1: value at time t of one bond bought at 0, if the yield is y from
    then on: coupons received before t are reinvested at y, the rest is priced at y."""
    return plain_sum(fl.C * math.exp(y * (t - fl.t)) if fl.t <= t else fl.C * math.exp(-y * (fl.t - t)) for fl in bond)


def log_return(bond: Bond, y0: float, y: float, t: float) -> float:
    """Annualised log return over [0,t] when buying at yield y0 and the yield jumps to y."""
    return math.log(investment_value(bond, y, t) / price(bond, y0)) / t


# 10.1.3 Portfolios of two bonds: the duration of a portfolio is the value-weighted
# average of durations. Weights w1 + w2 = 1 with w1 D1 + w2 D2 = D_target.
def immunising_weights(D1: float, D2: float, D_target: float) -> tuple[float, float]:
    w1 = (D_target - D2) / (D1 - D2)
    return w1, 1 - w1


@dataclass(frozen=True)
class Portfolio:
    q1: float  # number of bond 1
    q2: float  # number of bond 2
    cash: float


def value(p: Portfolio, b1: Bond, b2: Bond, y: float, t: float) -> float:
    return p.q1 * price(b1, y, t) + p.q2 * price(b2, y, t) + p.cash


def immunise(V: float, b1: Bond, b2: Bond, y: float, t: float, horizon: float) -> Portfolio:
    """Invest V at time t in bonds 1 and 2 so that the duration equals horizon - t."""
    w1, w2 = immunising_weights(duration(b1, y, t), duration(b2, y, t), horizon - t)
    return Portfolio(w1 * V / price(b1, y, t), w2 * V / price(b2, y, t), 0.0)


@dataclass(frozen=True)
class HedgeStep:
    t: float
    y: float
    value: float
    q1: float
    q2: float
    duration: float  # of the rebalanced portfolio


def dynamic_hedge(V0: float, b1: Bond, b2: Bond, horizon: float, path: list[tuple[float, float]],
                  rebalance: bool = True) -> list[HedgeStep]:
    """10.1.4: along a path of (t, y) with t[0] = 0, hold the two bonds, collect
    coupons into cash (accruing at the yield of the interval), and, if `rebalance`,
    re-immunise at each step so the duration equals the remaining horizon."""
    t0, y0 = path[0]
    p = immunise(V0, b1, b2, y0, t0, horizon)
    steps = [HedgeStep(t0, y0, V0, p.q1, p.q2, horizon - t0)]
    for i in range(1, len(path)):
        t_prev, y_prev = path[i - 1]
        t, y = path[i]
        cash = p.cash * math.exp(y_prev * (t - t_prev))
        for q, b in ((p.q1, b1), (p.q2, b2)):
            for fl in b:
                if t_prev < fl.t <= t:
                    cash += q * fl.C * math.exp(y_prev * (t - fl.t))
        p = Portfolio(p.q1, p.q2, cash)
        V = value(p, b1, b2, y, t)
        if rebalance and t < horizon:
            p = immunise(V, b1, b2, y, t, horizon)
        D = (p.q1 * price(b1, y, t) * duration(b1, y, t) + p.q2 * price(b2, y, t) * duration(b2, y, t)) / V \
            if t < horizon else 0.0
        steps.append(HedgeStep(t, y, V, p.q1, p.q2, D))
    return steps
