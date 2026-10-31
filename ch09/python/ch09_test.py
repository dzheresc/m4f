"""Unit tests for chapter 9."""

import math
import unittest

import black_scholes
import hedging
import risk

S, STRIKE, RATE, SIGMA, MATURITY = 100.0, 100.0, 0.05, 0.2, 0.5


class LibraryTest(unittest.TestCase):

    def test_norm(self) -> None:
        self.assertAlmostEqual(black_scholes.norm_cdf(0.0), 0.5)
        self.assertAlmostEqual(
            black_scholes.norm_cdf(1.959963984540054), 0.975, places=9
        )
        self.assertAlmostEqual(
            black_scholes.norm_inv(0.975), 1.959963984540054, places=9
        )
        self.assertAlmostEqual(
            black_scholes.norm_pdf(0.0), 1 / math.sqrt(2 * math.pi)
        )

    def test_norm_inv_against_bisection(self) -> None:
        """norm_inv against the bisection it replaced.

        Across the center, both tails and the region boundaries of the rational
        approximation. Near p = 1 the inverse is only defined to within one ulp
        of p divided by the density, so the tolerance includes that term.
        """

        def bisect(p: float) -> float:
            lo, hi = -40.0, 40.0
            for _ in range(200):
                mid = (lo + hi) / 2
                if black_scholes.norm_cdf(mid) < p:
                    lo = mid
                else:
                    hi = mid
            return (lo + hi) / 2

        for p in (1e-100, 1e-20, 1e-10, 1e-4, 0.001, 0.01, 0.02425, 0.0243,
                  0.1, 0.3, 0.5, 0.7, 0.95, 0.975, 0.97575, 0.99, 0.999,
                  0.999999):  # fmt: skip
            x = black_scholes.norm_inv(p)
            ref = bisect(p)
            self.assertLessEqual(
                abs(x - ref), 1e-13 + 4e-16 / black_scholes.norm_pdf(ref)
            )
            self.assertLessEqual(
                abs(black_scholes.norm_cdf(x) - p), 1e-14 * max(p, 1e-2)
            )
        self.assertEqual(black_scholes.norm_inv(0.0), -40.0)
        self.assertEqual(black_scholes.norm_inv(1.0), 40.0)
        # Beyond norm_cdf's range (|x| > 37) the result stays finite, ordered.
        self.assertTrue(-40 < black_scholes.norm_inv(1e-300) < -37)

    def test_greeks_by_finite_differences(self) -> None:
        h = 1e-4
        for is_call in (True, False):
            g = black_scholes.bs_greeks(
                S, STRIKE, RATE, SIGMA, MATURITY, is_call
            )

            def price(
                s=S, r=RATE, sigma=SIGMA, maturity=MATURITY, is_call=is_call
            ):
                return black_scholes.bs_price(
                    s, STRIKE, r, sigma, maturity, is_call
                )

            self.assertAlmostEqual(g.price, price())
            self.assertAlmostEqual(
                g.delta, (price(s=S + h) - price(s=S - h)) / (2 * h), places=6
            )
            self.assertAlmostEqual(
                g.gamma,
                (price(s=S + h) - 2 * price() + price(s=S - h)) / (h * h),
                places=4,
            )
            self.assertAlmostEqual(
                g.theta,
                -(price(maturity=MATURITY + h) - price(maturity=MATURITY - h))
                / (2 * h),
                places=5,
            )
            self.assertAlmostEqual(
                g.vega,
                (price(sigma=SIGMA + h) - price(sigma=SIGMA - h)) / (2 * h),
                places=5,
            )
            self.assertAlmostEqual(
                g.rho,
                (price(r=RATE + h) - price(r=RATE - h)) / (2 * h),
                places=5,
            )

    def test_hedges_are_neutral_and_self_financing(self) -> None:
        written = [hedging.Position(-1000.0, STRIKE, MATURITY, True)]
        extra = hedging.Position(0.0, 110.0, MATURITY, True)
        for h in (
            hedging.delta_hedge(written, S, RATE, SIGMA),
            hedging.delta_greek_hedge(written, extra, S, RATE, SIGMA, "gamma"),
            hedging.delta_greek_hedge(written, extra, S, RATE, SIGMA, "vega"),
        ):
            self.assertAlmostEqual(
                hedging.portfolio_value(h, S, 0.0, RATE, SIGMA), 0.0
            )
            self.assertAlmostEqual(
                hedging.total_greek(h.options, S, RATE, SIGMA, "delta")
                + h.shares,
                0.0,
            )
        h_gamma = hedging.delta_greek_hedge(
            written, extra, S, RATE, SIGMA, "gamma"
        )
        self.assertAlmostEqual(
            hedging.total_greek(h_gamma.options, S, RATE, SIGMA, "gamma"), 0.0
        )
        # Second order: the delta hedge error is about gamma/2 * dS^2.
        h_delta = hedging.delta_hedge(written, S, RATE, SIGMA)
        err = hedging.portfolio_value(h_delta, S + 1, 0.0, RATE, SIGMA)
        gamma = hedging.total_greek(written, S, RATE, SIGMA, "gamma")
        self.assertLess(abs(err - gamma / 2), 0.5)

    def test_rebalance_path_is_self_financing(self) -> None:
        written = [hedging.Position(-1000.0, STRIKE, MATURITY, True)]
        steps = hedging.rebalance_path(
            written, [100.0, 103.0, 101.0], 1 / 52, RATE, SIGMA
        )
        self.assertEqual(len(steps), 3)
        self.assertAlmostEqual(steps[0].value, 0.0)
        a, b = steps[0], steps[1]
        self.assertAlmostEqual(
            b.money,
            a.money * math.exp(RATE / 52) - (b.shares - a.shares) * b.s,
        )

    def test_lognormal(self) -> None:
        model = risk.Lognormal(100.0, 0.08, 0.3, 0.5)
        self.assertAlmostEqual(model.cdf(model.quantile(0.3)), 0.3)
        # Call and put expectations satisfy E S - X = E call - E put.
        self.assertAlmostEqual(
            model.expected_call_payoff(90.0) - model.expected_put_payoff(90.0),
            model.expected_stock() - 90.0,
        )
        # Expected call payoff by midpoint integration over Z.
        n, lo, hi = 20000, -8.0, 8.0
        total = 0.0
        for i in range(n):
            z = lo + (i + 0.5) * (hi - lo) / n
            s = 100.0 * math.exp(0.08 * 0.5 + 0.3 * math.sqrt(0.5) * z)
            total += (
                max(s - 90.0, 0.0) * black_scholes.norm_pdf(z) * (hi - lo) / n
            )
        self.assertAlmostEqual(
            total, model.expected_call_payoff(90.0), places=6
        )

    def test_var_and_strategies(self) -> None:
        model = risk.Lognormal(100.0, 0.08, 0.3, 0.5)
        stock = risk.unhedged_holding(1.0)
        self.assertAlmostEqual(
            risk.value_at_risk(stock, model, RATE, 0.3, 0.95),
            100.0 - model.quantile(0.05),
        )
        hedged = risk.put_hedged_holding("p", 1.0, 95.0)
        self.assertAlmostEqual(
            risk.value_at_risk(hedged, model, RATE, 0.3, 0.95),
            black_scholes.bs_price(100.0, 95.0, RATE, 0.3, 0.5, False) + 5.0,
        )
        self.assertLess(
            risk.expected_return(hedged, model, RATE, 0.3),
            risk.expected_return(stock, model, RATE, 0.3),
        )
        v0 = 10000.0
        for st in (
            risk.all_in_stock(v0, 100.0),
            risk.all_in_calls("c", v0, 100.0, 100.0, RATE, 0.25, 0.5),
            risk.bull_spread("b", v0, 100.0, 100.0, 115.0, RATE, 0.25, 0.5),
        ):
            self.assertAlmostEqual(
                risk.initial_cost(st, 100.0, RATE, 0.25, 0.5), v0
            )
            be = risk.breakeven(st, model, RATE, 0.25)
            self.assertAlmostEqual(
                st.terminal_value(be, RATE, 0.5), v0, places=6
            )
            self.assertAlmostEqual(
                risk.prob_loss(st, model, RATE, 0.25), model.cdf(be)
            )
        self.assertAlmostEqual(
            risk.breakeven(
                risk.all_in_calls("c", v0, 100.0, 100.0, RATE, 0.25, 0.5),
                model,
                RATE,
                0.25,
            ),
            100.0 + black_scholes.bs_price(100.0, 100.0, RATE, 0.25, 0.5, True),
        )


if __name__ == "__main__":
    unittest.main()
