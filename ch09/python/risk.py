"""Sections 9.2 and 9.3: Value at Risk and speculation under a lognormal stock.

    S(T) = S(0) exp(m T + sigma sqrt(T) Z),  Z ~ N(0,1),

so ln S(T)/S(0) is normal with mean m T and variance sigma^2 T. m is the
expected logarithmic return; the Black-Scholes r only enters option prices.

A Strategy is shares + cash + long/short options held to T. Its terminal value
V(S(T)) is assumed non-decreasing in S(T) (long stock, long calls, long puts
hedging stock, bull spreads), so quantiles of V are V at quantiles of S(T).
"""

import dataclasses
import math

import black_scholes


@dataclasses.dataclass(frozen=True)
class Lognormal:
    """The lognormal model of S(T)."""

    s0: float
    m: float
    sigma: float
    maturity: float

    def quantile(self, p: float) -> float:
        """The p-quantile of S(T)."""
        return self.s0 * math.exp(
            self.m * self.maturity
            + self.sigma * math.sqrt(self.maturity) * black_scholes.norm_inv(p)
        )

    def cdf(self, s: float) -> float:
        """P(S(T) <= s)."""
        return black_scholes.norm_cdf(
            (math.log(s / self.s0) - self.m * self.maturity)
            / (self.sigma * math.sqrt(self.maturity))
        )

    def expected_stock(self) -> float:
        """E S(T)."""
        return self.s0 * math.exp(
            self.m * self.maturity + self.sigma * self.sigma * self.maturity / 2
        )

    def expected_call_payoff(self, strike: float) -> float:
        """E max(S(T)-X, 0) = S0 e^((m+sigma^2/2)T) N(delta1) - X N(delta2)."""
        sq = self.sigma * math.sqrt(self.maturity)
        delta2 = (math.log(self.s0 / strike) + self.m * self.maturity) / sq
        delta1 = delta2 + sq
        return self.expected_stock() * black_scholes.norm_cdf(
            delta1
        ) - strike * black_scholes.norm_cdf(delta2)

    def expected_put_payoff(self, strike: float) -> float:
        """E max(X-S(T), 0) = E max(S(T)-X, 0) - E S(T) + X."""
        return (
            self.expected_call_payoff(strike) - self.expected_stock() + strike
        )


@dataclasses.dataclass(frozen=True)
class OptionLeg:
    qty: float
    strike: float
    is_call: bool


@dataclasses.dataclass(frozen=True)
class Strategy:
    """Shares, cash and options held to T.

    Attributes:
        name: Label used in the report.
        shares: Number of shares.
        cash: Invested in the money market at r.
        legs: Long (qty > 0) or short option positions.
    """

    name: str
    shares: float
    cash: float
    legs: list[OptionLeg]

    def terminal_value(self, s: float, r: float, maturity: float) -> float:
        """Value at T when the stock ends at s."""
        v = self.shares * s + self.cash * math.exp(r * maturity)
        for leg in self.legs:
            v += leg.qty * black_scholes.payoff(s, leg.strike, leg.is_call)
        return v

    def expected_value(self, model: Lognormal, r: float) -> float:
        """E V(T) under the lognormal model."""
        v = self.shares * model.expected_stock() + self.cash * math.exp(
            r * model.maturity
        )
        for leg in self.legs:
            if leg.is_call:
                v += leg.qty * model.expected_call_payoff(leg.strike)
            else:
                v += leg.qty * model.expected_put_payoff(leg.strike)
        return v


def initial_cost(
    st: Strategy, s0: float, r: float, sigma: float, maturity: float
) -> float:
    """Cost of setting up the strategy at time 0."""
    c = st.shares * s0 + st.cash
    for leg in st.legs:
        c += leg.qty * black_scholes.bs_price(
            s0, leg.strike, r, sigma, maturity, leg.is_call
        )
    return c


def value_at_risk(
    st: Strategy, model: Lognormal, r: float, sigma: float, conf: float
) -> float:
    """Section 9.2.1: VaR at confidence level conf.

    The initial value minus the (1-conf) quantile of the terminal value
    (positive = a loss).
    """
    v0 = initial_cost(st, model.s0, r, sigma, model.maturity)
    return v0 - st.terminal_value(model.quantile(1 - conf), r, model.maturity)


def expected_return(
    st: Strategy, model: Lognormal, r: float, sigma: float
) -> float:
    """E V(T) / V(0) - 1."""
    v0 = initial_cost(st, model.s0, r, sigma, model.maturity)
    return st.expected_value(model, r) / v0 - 1


def breakeven(st: Strategy, model: Lognormal, r: float, sigma: float) -> float:
    """Stock price at T where the strategy breaks even (by bisection)."""
    v0 = initial_cost(st, model.s0, r, sigma, model.maturity)
    lo, hi = 0.0, 100.0 * model.s0
    for _ in range(200):
        mid = (lo + hi) / 2
        if st.terminal_value(mid, r, model.maturity) < v0:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2


def prob_loss(st: Strategy, model: Lognormal, r: float, sigma: float) -> float:
    """P(V(T) < V(0))."""
    return model.cdf(breakeven(st, model, r, sigma))


# Section 9.2: a holding of `shares` shares, hedged by buying `h` puts per
# share at the given strike.
def put_hedged_holding(
    name: str, shares: float, strike: float, h: float = 1.0
) -> Strategy:
    return Strategy(name, shares, 0.0, [OptionLeg(h * shares, strike, False)])


def unhedged_holding(shares: float) -> Strategy:
    return Strategy("stock", shares, 0.0, [])


# Section 9.3.1: spend capital v0 on one instrument at stock price s0 = S(0).
def all_in_stock(v0: float, s0: float) -> Strategy:
    return Strategy("stock", v0 / s0, 0.0, [])


def all_in_calls(
    name: str,
    v0: float,
    s0: float,
    strike: float,
    r: float,
    sigma: float,
    maturity: float,
) -> Strategy:
    n = v0 / black_scholes.bs_price(s0, strike, r, sigma, maturity, True)
    return Strategy(name, 0.0, 0.0, [OptionLeg(n, strike, True)])


def bull_spread(
    name: str,
    v0: float,
    s0: float,
    x1: float,
    x2: float,
    r: float,
    sigma: float,
    maturity: float,
) -> Strategy:
    """Buys calls at strike x1 and writes as many at x2 (x1 < x2).

    Buys as many spreads as v0 pays for.
    """
    n = v0 / (
        black_scholes.bs_price(s0, x1, r, sigma, maturity, True)
        - black_scholes.bs_price(s0, x2, r, sigma, maturity, True)
    )
    return Strategy(
        name, 0.0, 0.0, [OptionLeg(n, x1, True), OptionLeg(-n, x2, True)]
    )


def mix(name: str, a: Strategy, wa: float, b: Strategy, wb: float) -> Strategy:
    """Holds strategy `a` scaled by `wa` plus strategy `b` scaled by `wb`."""
    legs = [OptionLeg(leg.qty * wa, leg.strike, leg.is_call) for leg in a.legs]
    legs += [OptionLeg(leg.qty * wb, leg.strike, leg.is_call) for leg in b.legs]
    return Strategy(
        name, a.shares * wa + b.shares * wb, a.cash * wa + b.cash * wb, legs
    )
