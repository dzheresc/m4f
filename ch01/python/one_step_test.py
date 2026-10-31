"""Unit tests for chapter 1."""

import unittest

import forward
import fx_arbitrage
import one_step

OneStepMarket = one_step.OneStepMarket
Portfolio = one_step.Portfolio
UP = one_step.State.UP
DOWN = one_step.State.DOWN


class OneStepMarketTest(unittest.TestCase):

    def setUp(self) -> None:
        super().setUp()
        self.market = OneStepMarket(100, 110, 50, 60, 40, 0.8)

    def test_returns(self) -> None:
        self.assertAlmostEqual(self.market.r, 0.1)
        self.assertAlmostEqual(self.market.u, 0.2)
        self.assertAlmostEqual(self.market.d, -0.2)
        self.assertTrue(self.market.is_arbitrage_free)

    def test_risk_neutral_and_forward(self) -> None:
        self.assertAlmostEqual(self.market.risk_neutral_prob, 0.75)
        self.assertAlmostEqual(self.market.forward_price, 55)

    def test_call_replication(self) -> None:
        call = self.market.replicate(10, 0)
        self.assertAlmostEqual(call.x, 0.5)
        self.assertAlmostEqual(call.y, -2 / 11)
        self.assertAlmostEqual(self.market.price(10, 0), 75 / 11)
        self.assertAlmostEqual(
            self.market.price(10, 0), self.market.risk_neutral_price(10, 0)
        )

    def test_put_call_parity(self) -> None:
        self.assertAlmostEqual(
            self.market.price(10, 0) - self.market.price(0, 10), 50 - 50 / 1.1
        )

    def test_book_examples_and_exercises(self) -> None:
        ex = OneStepMarket(100, 110, 50, 52, 48, 0.5)
        self.assertAlmostEqual(ex.u, 0.04)
        self.assertAlmostEqual(ex.d, -0.04)
        self.assertFalse(ex.is_arbitrage_free)
        portfolio = Portfolio(20, 10)
        self.assertAlmostEqual(ex.value0(portfolio), 2000)
        self.assertAlmostEqual(ex.value1(portfolio, UP), 2140)
        self.assertAlmostEqual(ex.value1(portfolio, DOWN), 2060)
        self.assertAlmostEqual(ex.portfolio_return(portfolio, UP), 0.07)
        self.assertAlmostEqual(ex.portfolio_return(portfolio, DOWN), 0.03)

        er = OneStepMarket(90, 100, 25, 30, 20, 0.5)
        other = Portfolio(10, 15)
        self.assertAlmostEqual(er.value0(other), 1600)
        self.assertAlmostEqual(er.value1(other, UP), 1800)
        self.assertAlmostEqual(er.value1(other, DOWN), 1700)
        replica = er.replicate(1160, 1040)
        self.assertAlmostEqual(replica.x, 12)
        self.assertAlmostEqual(replica.y, 8)
        self.assertAlmostEqual(er.value0(replica), 1020)

    def test_currency_arbitrage(self) -> None:
        fx = fx_arbitrage.FxMarket(
            [
                fx_arbitrage.FxQuote("A", "EUR", "USD", 1.0202, 1.0284),
                fx_arbitrage.FxQuote("A", "GBP", "USD", 1.5718, 1.5844),
                fx_arbitrage.FxQuote("B", "EUR", "GBP", 0.6324, 0.6401),
                fx_arbitrage.FxQuote("B", "USD", "GBP", 0.6299, 0.6375),
            ]
        )
        cycle = fx.best_cycle("USD")
        self.assertEqual([leg.to for leg in cycle.legs], ["GBP", "EUR", "USD"])
        self.assertAlmostEqual(cycle.growth, 1.0202 / (1.5844 * 0.6401))

    def test_example_1_4(self) -> None:
        market = OneStepMarket(100, 110, 100, 125, 105, 0.5)
        self.assertAlmostEqual(market.u, 0.25)
        self.assertAlmostEqual(market.d, 0.05)
        self.assertTrue(market.is_arbitrage_free)
        self.assertAlmostEqual(market.risk_neutral_prob, 0.25)
        self.assertAlmostEqual(market.forward_price, 110)

    def test_risk_and_return(self) -> None:
        market = OneStepMarket(100, 110, 80, 100, 60, 0.8)

        def assert_moments(
            portfolio: one_step.Portfolio, mean: float, sd: float
        ) -> None:
            k_up = market.portfolio_return(portfolio, UP)
            k_down = market.portfolio_return(portfolio, DOWN)
            self.assertAlmostEqual(market.expect(k_up, k_down), mean)
            self.assertAlmostEqual(market.stddev(k_up, k_down), sd)

        assert_moments(Portfolio(50, 60), 0.12, 0.08)
        assert_moments(Portfolio(0, 100), 0.10, 0.0)
        assert_moments(Portfolio(125, 0), 0.15, 0.20)
        half = market.split(10000, 0.5)
        self.assertAlmostEqual(half.x, 62.5)
        self.assertAlmostEqual(half.y, 50)
        assert_moments(half, 0.125, 0.10)

    def test_forwards(self) -> None:
        arb = forward.forward_arbitrage(34, 100, 112, 38.60)
        self.assertIsNotNone(arb)
        self.assertAlmostEqual(arb.value0(34, 100), 0)
        self.assertAlmostEqual(arb.value1(30, 112, 38.60), 0.52)
        self.assertAlmostEqual(arb.value1(50, 112, 38.60), 0.52)
        self.assertIsNone(forward.forward_arbitrage(34, 100, 112, 38.08))
        self.assertAlmostEqual(
            forward.implied_foreign_rate(1.6, 1.5, 0.05), 0.12
        )

    def test_options(self) -> None:
        market = OneStepMarket(100, 110, 100, 120, 80, 0.5)
        self.assertAlmostEqual(market.price(20, 0), 150 / 11)  # Call X=100.
        self.assertAlmostEqual(market.price(30, 0), 225 / 11)  # Ex. 1.7a.
        self.assertAlmostEqual(market.price(10, 0), 75 / 11)  # Ex. 1.7b.
        self.assertAlmostEqual(  # Exercise 1.8.
            OneStepMarket(100, 105, 100, 120, 80, 0.5).price(20, 0),
            50 - 4000 / 105,
        )
        self.assertAlmostEqual(
            OneStepMarket(100, 115, 100, 120, 80, 0.5).price(20, 0),
            50 - 4000 / 115,
        )
        self.assertAlmostEqual(market.price(0, 20), 50 / 11)  # Exercise 1.9.

    def test_hedging(self) -> None:
        cost = one_step.hedged_purchase_cost
        market = OneStepMarket(100, 110, 100, 160, 40, 0.5)
        self.assertAlmostEqual(market.price(60, 0), 350 / 11)
        self.assertAlmostEqual(cost(market, 100, 1, UP), 135)
        self.assertAlmostEqual(cost(market, 100, 1, DOWN), 75)
        self.assertAlmostEqual(cost(market, 100, 2, UP), 110)
        self.assertAlmostEqual(cost(market, 100, 2, DOWN), 110)
        for p in (0.1, 0.25, 0.5, 0.75, 0.9):  # Exercise 1.12.
            market_p = OneStepMarket(100, 110, 100, 160, 40, p)
            sigma_with = market_p.stddev(
                cost(market_p, 100, 1, UP), cost(market_p, 100, 1, DOWN)
            )
            self.assertAlmostEqual(sigma_with, market_p.stddev(160, 40) / 2)

    def test_arbitrage(self) -> None:
        self.assertIsNone(self.market.arbitrage())
        for s0 in (55.0, 36.0):
            bad = OneStepMarket(100, 110, s0, 60, 40, 0.8)
            arb = bad.arbitrage()
            self.assertIsNotNone(arb)
            self.assertAlmostEqual(bad.value0(arb), 0)
            v_up, v_down = bad.value1(arb, UP), bad.value1(arb, DOWN)
            self.assertTrue(v_up >= 0 and v_down >= 0 and v_up + v_down > 0)


if __name__ == "__main__":
    unittest.main()
