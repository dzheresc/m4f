"""Sections 9.2 and 9.3: Value at Risk and speculation under a lognormal stock,
    S(T) = S(0) exp(m T + sigma sqrt(T) Z),  Z ~ N(0,1),
so ln S(T)/S(0) is normal with mean m T and variance sigma^2 T. m is the
expected logarithmic return; the Black-Scholes r only enters option prices.

A Strategy is shares + cash + long/short options held to T. Its terminal value
V(S(T)) is assumed non-decreasing in S(T) (long stock, long calls, long puts
hedging stock, bull spreads), so quantiles of V are V at quantiles of S(T).
"""
from __future__ import annotations

import math
from dataclasses import dataclass

from black_scholes import bs_price, norm_cdf, norm_inv, payoff


@dataclass(frozen=True)
class Lognormal:
    S0: float
    m: float
    sigma: float
    T: float

    def quantile(self, p: float) -> float:
        return self.S0 * math.exp(self.m * self.T + self.sigma * math.sqrt(self.T) * norm_inv(p))

    def cdf(self, s: float) -> float:
        """P(S(T) <= s)"""
        return norm_cdf((math.log(s / self.S0) - self.m * self.T) / (self.sigma * math.sqrt(self.T)))

    def expected_stock(self) -> float:
        return self.S0 * math.exp(self.m * self.T + self.sigma * self.sigma * self.T / 2)

    def expected_call_payoff(self, X: float) -> float:
        """E max(S(T)-X, 0) = S0 e^((m+sigma^2/2)T) N(delta1) - X N(delta2)"""
        sq = self.sigma * math.sqrt(self.T)
        delta2 = (math.log(self.S0 / X) + self.m * self.T) / sq
        delta1 = delta2 + sq
        return self.expected_stock() * norm_cdf(delta1) - X * norm_cdf(delta2)

    def expected_put_payoff(self, X: float) -> float:
        """E max(X-S(T), 0) = E max(S(T)-X, 0) - E S(T) + X"""
        return self.expected_call_payoff(X) - self.expected_stock() + X


@dataclass(frozen=True)
class OptionLeg:
    qty: float
    X: float
    is_call: bool


@dataclass(frozen=True)
class Strategy:
    name: str
    shares: float
    cash: float            # invested in the money market at r
    legs: list[OptionLeg]

    def terminal_value(self, S: float, r: float, T: float) -> float:
        v = self.shares * S + self.cash * math.exp(r * T)
        for leg in self.legs:
            v += leg.qty * payoff(S, leg.X, leg.is_call)
        return v

    def expected_value(self, model: Lognormal, r: float) -> float:
        v = self.shares * model.expected_stock() + self.cash * math.exp(r * model.T)
        for leg in self.legs:
            v += leg.qty * (model.expected_call_payoff(leg.X) if leg.is_call else model.expected_put_payoff(leg.X))
        return v


def initial_cost(st: Strategy, S0: float, r: float, sigma: float, T: float) -> float:
    c = st.shares * S0 + st.cash
    for leg in st.legs:
        c += leg.qty * bs_price(S0, leg.X, r, sigma, T, leg.is_call)
    return c


def value_at_risk(st: Strategy, model: Lognormal, r: float, sigma: float, conf: float) -> float:
    """9.2.1: VaR at confidence level conf = initial value minus the (1-conf)
    quantile of the terminal value (positive = a loss)."""
    V0 = initial_cost(st, model.S0, r, sigma, model.T)
    return V0 - st.terminal_value(model.quantile(1 - conf), r, model.T)


def expected_return(st: Strategy, model: Lognormal, r: float, sigma: float) -> float:
    V0 = initial_cost(st, model.S0, r, sigma, model.T)
    return st.expected_value(model, r) / V0 - 1


def breakeven(st: Strategy, model: Lognormal, r: float, sigma: float) -> float:
    """Stock price at T where the strategy neither gains nor loses (bisection)."""
    V0 = initial_cost(st, model.S0, r, sigma, model.T)
    lo, hi = 0.0, 100.0 * model.S0
    for _ in range(200):
        mid = (lo + hi) / 2
        if st.terminal_value(mid, r, model.T) < V0:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2


def prob_loss(st: Strategy, model: Lognormal, r: float, sigma: float) -> float:
    return model.cdf(breakeven(st, model, r, sigma))


# 9.2: a holding of `shares` shares, hedged by buying `h` puts per share at strike X.
def put_hedged_holding(name: str, shares: float, X: float, h: float = 1.0) -> Strategy:
    return Strategy(name, shares, 0.0, [OptionLeg(h * shares, X, False)])


def unhedged_holding(shares: float) -> Strategy:
    return Strategy("stock", shares, 0.0, [])


# 9.3.1: spend capital V0 on one instrument.
def all_in_stock(V0: float, S0: float) -> Strategy:
    return Strategy("stock", V0 / S0, 0.0, [])


def all_in_calls(name: str, V0: float, S0: float, X: float, r: float, sigma: float, T: float) -> Strategy:
    n = V0 / bs_price(S0, X, r, sigma, T, True)
    return Strategy(name, 0.0, 0.0, [OptionLeg(n, X, True)])


def bull_spread(name: str, V0: float, S0: float, X1: float, X2: float, r: float, sigma: float, T: float) -> Strategy:
    n = V0 / (bs_price(S0, X1, r, sigma, T, True) - bs_price(S0, X2, r, sigma, T, True))
    return Strategy(name, 0.0, 0.0, [OptionLeg(n, X1, True), OptionLeg(-n, X2, True)])


def mix(name: str, a: Strategy, wa: float, b: Strategy, wb: float) -> Strategy:
    legs = [OptionLeg(l.qty * wa, l.X, l.is_call) for l in a.legs] + [OptionLeg(l.qty * wb, l.X, l.is_call) for l in b.legs]
    return Strategy(name, a.shares * wa + b.shares * wb, a.cash * wa + b.cash * wb, legs)
