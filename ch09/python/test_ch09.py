import math
import unittest

from black_scholes import bs_greeks, bs_price, norm_cdf, norm_inv, norm_pdf
from hedging import Position, delta_greek_hedge, delta_hedge, portfolio_value, rebalance_path, total_greek
from risk import (Lognormal, all_in_calls, all_in_stock, breakeven, bull_spread, expected_return, initial_cost,
                  prob_loss, put_hedged_holding, unhedged_holding, value_at_risk)

S, X, r, sigma, T = 100.0, 100.0, 0.05, 0.2, 0.5


class LibraryTest(unittest.TestCase):
    def test_norm(self) -> None:
        self.assertAlmostEqual(norm_cdf(0.0), 0.5)
        self.assertAlmostEqual(norm_cdf(1.959963984540054), 0.975, places=9)
        self.assertAlmostEqual(norm_inv(0.975), 1.959963984540054, places=9)
        self.assertAlmostEqual(norm_pdf(0.0), 1 / math.sqrt(2 * math.pi))

    def test_greeks_by_finite_differences(self) -> None:
        h = 1e-4
        for is_call in (True, False):
            g = bs_greeks(S, X, r, sigma, T, is_call)
            p = lambda s=S, x=X, rr=r, sg=sigma, t=T: bs_price(s, x, rr, sg, t, is_call)  # noqa: E731
            self.assertAlmostEqual(g.price, p())
            self.assertAlmostEqual(g.delta, (p(s=S + h) - p(s=S - h)) / (2 * h), places=6)
            self.assertAlmostEqual(g.gamma, (p(s=S + h) - 2 * p() + p(s=S - h)) / (h * h), places=4)
            self.assertAlmostEqual(g.theta, -(p(t=T + h) - p(t=T - h)) / (2 * h), places=5)
            self.assertAlmostEqual(g.vega, (p(sg=sigma + h) - p(sg=sigma - h)) / (2 * h), places=5)
            self.assertAlmostEqual(g.rho, (p(rr=r + h) - p(rr=r - h)) / (2 * h), places=5)

    def test_hedges_are_neutral_and_self_financing(self) -> None:
        written = [Position(-1000.0, X, T, True)]
        extra = Position(0.0, 110.0, T, True)
        for h in (delta_hedge(written, S, r, sigma), delta_greek_hedge(written, extra, S, r, sigma, "gamma"),
                  delta_greek_hedge(written, extra, S, r, sigma, "vega")):
            self.assertAlmostEqual(portfolio_value(h, S, 0.0, r, sigma), 0.0)
            self.assertAlmostEqual(total_greek(h.options, S, r, sigma, "delta") + h.shares, 0.0)
        hg = delta_greek_hedge(written, extra, S, r, sigma, "gamma")
        self.assertAlmostEqual(total_greek(hg.options, S, r, sigma, "gamma"), 0.0)
        # second-order: delta hedge error ~ gamma/2 * dS^2
        hd = delta_hedge(written, S, r, sigma)
        err = portfolio_value(hd, S + 1, 0.0, r, sigma)
        self.assertLess(abs(err - total_greek(written, S, r, sigma, "gamma") / 2), 0.5)

    def test_rebalance_path_is_self_financing(self) -> None:
        written = [Position(-1000.0, X, T, True)]
        steps = rebalance_path(written, [100.0, 103.0, 101.0], 1 / 52, r, sigma)
        self.assertEqual(len(steps), 3)
        self.assertAlmostEqual(steps[0].value, 0.0)
        a, b = steps[0], steps[1]
        self.assertAlmostEqual(b.money, a.money * math.exp(r / 52) - (b.shares - a.shares) * b.S)

    def test_lognormal(self) -> None:
        m = Lognormal(100.0, 0.08, 0.3, 0.5)
        self.assertAlmostEqual(m.cdf(m.quantile(0.3)), 0.3)
        # call + put expectations satisfy E S - X = E call - E put
        self.assertAlmostEqual(m.expected_call_payoff(90.0) - m.expected_put_payoff(90.0), m.expected_stock() - 90.0)
        # numerical expectation of the call payoff by midpoint integration over Z
        n, lo, hi = 20000, -8.0, 8.0
        tot = 0.0
        for i in range(n):
            z = lo + (i + 0.5) * (hi - lo) / n
            s = 100.0 * math.exp(0.08 * 0.5 + 0.3 * math.sqrt(0.5) * z)
            tot += max(s - 90.0, 0.0) * norm_pdf(z) * (hi - lo) / n
        self.assertAlmostEqual(tot, m.expected_call_payoff(90.0), places=6)

    def test_var_and_strategies(self) -> None:
        m = Lognormal(100.0, 0.08, 0.3, 0.5)
        stock = unhedged_holding(1.0)
        self.assertAlmostEqual(value_at_risk(stock, m, r, 0.3, 0.95), 100.0 - m.quantile(0.05))
        hedged = put_hedged_holding("p", 1.0, 95.0)
        self.assertAlmostEqual(value_at_risk(hedged, m, r, 0.3, 0.95), bs_price(100.0, 95.0, r, 0.3, 0.5, False) + 5.0)
        self.assertLess(expected_return(hedged, m, r, 0.3), expected_return(stock, m, r, 0.3))
        V0 = 10000.0
        for st in (all_in_stock(V0, 100.0), all_in_calls("c", V0, 100.0, 100.0, r, 0.25, 0.5),
                   bull_spread("b", V0, 100.0, 100.0, 115.0, r, 0.25, 0.5)):
            self.assertAlmostEqual(initial_cost(st, 100.0, r, 0.25, 0.5), V0)
            be = breakeven(st, m, r, 0.25)
            self.assertAlmostEqual(st.terminal_value(be, r, 0.5), V0, places=6)
            self.assertAlmostEqual(prob_loss(st, m, r, 0.25), m.cdf(be))
        self.assertAlmostEqual(breakeven(all_in_calls("c", V0, 100.0, 100.0, r, 0.25, 0.5), m, r, 0.25),
                               100.0 + bs_price(100.0, 100.0, r, 0.25, 0.5, True))


if __name__ == "__main__":
    unittest.main()
