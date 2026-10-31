"""Section 10.1: maturity-independent yields.

All bonds are priced with one continuously compounded yield y, so
B(t,T) = e^(-y(T-t)). A Bond is a list of cash flows (t_i, C_i); coupons and
the face value are flows.
"""

from collections.abc import Iterable
import dataclasses
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


@dataclasses.dataclass(frozen=True)
class Flow:
    t: float
    amount: float


Bond = list[Flow]


def zero_coupon(face: float, maturity: float) -> Bond:
    return [Flow(maturity, face)]


def coupon_bond(face: float, coupon: float, n: int, dt: float = 1.0) -> Bond:
    """n coupons at dt, 2dt, ..., n dt; the face is repaid with the last."""
    return [
        Flow((i + 1) * dt, coupon + (face if i == n - 1 else 0.0))
        for i in range(n)
    ]


def price(bond: Bond, y: float, t: float = 0.0) -> float:
    """P(t) = sum over t_i > t of C_i e^(-y(t_i - t))."""
    return plain_sum(
        flow.amount * math.exp(-y * (flow.t - t)) for flow in bond if flow.t > t
    )


def duration(bond: Bond, y: float, t: float = 0.0) -> float:
    """D = sum (t_i - t) C_i e^(-y(t_i - t)) / P; dP/dy = -D P."""
    return plain_sum(
        (flow.t - t) * flow.amount * math.exp(-y * (flow.t - t))
        for flow in bond
        if flow.t > t
    ) / price(bond, y, t)


def investment_value(bond: Bond, y: float, t: float) -> float:
    """Section 10.1.1: value at time t of one bond bought at 0.

    The yield is y from then on: coupons received before t are reinvested at
    y, the rest is priced at y.
    """
    return plain_sum(
        (
            flow.amount * math.exp(y * (t - flow.t))
            if flow.t <= t
            else flow.amount * math.exp(-y * (flow.t - t))
        )
        for flow in bond
    )


def log_return(bond: Bond, y0: float, y: float, t: float) -> float:
    """Annualised log return over [0,t], bought at y0 when y0 jumps to y."""
    return math.log(investment_value(bond, y, t) / price(bond, y0)) / t


# Section 10.1.3: portfolios of two bonds. The duration of a portfolio is the
# value-weighted average of durations. Weights w1 + w2 = 1 with
# w1 D1 + w2 D2 = D_target.
def immunising_weights(
    d1: float, d2: float, d_target: float
) -> tuple[float, float]:
    w1 = (d_target - d2) / (d1 - d2)
    return w1, 1 - w1


@dataclasses.dataclass(frozen=True)
class Portfolio:
    """Holdings of two bonds plus cash.

    Attributes:
        q1: Number of bond 1.
        q2: Number of bond 2.
        cash: Cash balance.
    """

    q1: float
    q2: float
    cash: float


def portfolio_value(
    p: Portfolio, b1: Bond, b2: Bond, y: float, t: float
) -> float:
    return p.q1 * price(b1, y, t) + p.q2 * price(b2, y, t) + p.cash


def immunise(
    v: float, b1: Bond, b2: Bond, y: float, t: float, horizon: float
) -> Portfolio:
    """Invest v at time t in bonds 1 and 2 with duration horizon - t."""
    w1, w2 = immunising_weights(
        duration(b1, y, t), duration(b2, y, t), horizon - t
    )
    return Portfolio(w1 * v / price(b1, y, t), w2 * v / price(b2, y, t), 0.0)


@dataclasses.dataclass(frozen=True)
class HedgeStep:
    """One step of the dynamic hedge.

    Attributes:
        t: Time.
        y: Yield.
        value: Portfolio value.
        q1: Number of bond 1 after rebalancing.
        q2: Number of bond 2 after rebalancing.
        duration: Duration of the rebalanced portfolio.
    """

    t: float
    y: float
    value: float
    q1: float
    q2: float
    duration: float


def dynamic_hedge(
    v0: float,
    b1: Bond,
    b2: Bond,
    horizon: float,
    path: list[tuple[float, float]],
    rebalance: bool = True,
) -> list[HedgeStep]:
    """Section 10.1.4: hedge along a path of (t, y) with t[0] = 0.

    Holds the two bonds, collects coupons into cash (accruing at the yield of
    the interval), and, if `rebalance`, re-immunises at each step so the
    duration equals the remaining horizon.
    """
    t0, y0 = path[0]
    p = immunise(v0, b1, b2, y0, t0, horizon)
    steps = [HedgeStep(t0, y0, v0, p.q1, p.q2, horizon - t0)]
    for i in range(1, len(path)):
        t_prev, y_prev = path[i - 1]
        t, y = path[i]
        cash = p.cash * math.exp(y_prev * (t - t_prev))
        for q, b in ((p.q1, b1), (p.q2, b2)):
            for flow in b:
                if t_prev < flow.t <= t:
                    cash += q * flow.amount * math.exp(y_prev * (t - flow.t))
        p = Portfolio(p.q1, p.q2, cash)
        v = portfolio_value(p, b1, b2, y, t)
        if rebalance and t < horizon:
            p = immunise(v, b1, b2, y, t, horizon)
        if t < horizon:
            d = (
                p.q1 * price(b1, y, t) * duration(b1, y, t)
                + p.q2 * price(b2, y, t) * duration(b2, y, t)
            ) / v
        else:
            d = 0.0
        steps.append(HedgeStep(t, y, v, p.q1, p.q2, d))
    return steps
