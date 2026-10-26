import unittest

from forward import forward_arbitrage, implied_foreign_rate
from fx_arbitrage import FxMarket, FxQuote
from one_step import OneStepMarket, Portfolio, State, hedged_purchase_cost


class OneStepMarketTest(unittest.TestCase):
    def setUp(self) -> None:
        self.m = OneStepMarket(100, 110, 50, 60, 40, 0.8)

    def test_returns(self) -> None:
        m = self.m
        self.assertAlmostEqual(m.r, 0.1)
        self.assertAlmostEqual(m.u, 0.2)
        self.assertAlmostEqual(m.d, -0.2)
        self.assertTrue(m.is_arbitrage_free)

    def test_risk_neutral_and_forward(self) -> None:
        self.assertAlmostEqual(self.m.risk_neutral_prob, 0.75)
        self.assertAlmostEqual(self.m.forward_price, 55)

    def test_call_replication(self) -> None:
        c = self.m.replicate(10, 0)
        self.assertAlmostEqual(c.x, 0.5)
        self.assertAlmostEqual(c.y, -2 / 11)
        self.assertAlmostEqual(self.m.price(10, 0), 75 / 11)
        self.assertAlmostEqual(self.m.price(10, 0), self.m.risk_neutral_price(10, 0))

    def test_put_call_parity(self) -> None:
        self.assertAlmostEqual(self.m.price(10, 0) - self.m.price(0, 10), 50 - 50 / 1.1)

    def test_book_examples_and_exercises(self) -> None:
        ex = OneStepMarket(100, 110, 50, 52, 48, 0.5)
        self.assertAlmostEqual(ex.u, 0.04)
        self.assertAlmostEqual(ex.d, -0.04)
        self.assertFalse(ex.is_arbitrage_free)
        P = Portfolio(20, 10)
        self.assertAlmostEqual(ex.value0(P), 2000)
        self.assertAlmostEqual(ex.value1(P, State.UP), 2140)
        self.assertAlmostEqual(ex.value1(P, State.DOWN), 2060)
        self.assertAlmostEqual(ex.portfolio_return(P, State.UP), 0.07)
        self.assertAlmostEqual(ex.portfolio_return(P, State.DOWN), 0.03)

        er = OneStepMarket(90, 100, 25, 30, 20, 0.5)
        Q = Portfolio(10, 15)
        self.assertAlmostEqual(er.value0(Q), 1600)
        self.assertAlmostEqual(er.value1(Q, State.UP), 1800)
        self.assertAlmostEqual(er.value1(Q, State.DOWN), 1700)
        R = er.replicate(1160, 1040)
        self.assertAlmostEqual(R.x, 12)
        self.assertAlmostEqual(R.y, 8)
        self.assertAlmostEqual(er.value0(R), 1020)

    def test_currency_arbitrage(self) -> None:
        fx = FxMarket([FxQuote("A", "EUR", "USD", 1.0202, 1.0284),
                       FxQuote("A", "GBP", "USD", 1.5718, 1.5844),
                       FxQuote("B", "EUR", "GBP", 0.6324, 0.6401),
                       FxQuote("B", "USD", "GBP", 0.6299, 0.6375)])
        c = fx.best_cycle("USD")
        self.assertEqual([l.to for l in c.legs], ["GBP", "EUR", "USD"])
        self.assertAlmostEqual(c.growth, 1.0202 / (1.5844 * 0.6401))

    def test_example_1_4(self) -> None:
        m4 = OneStepMarket(100, 110, 100, 125, 105, 0.5)
        self.assertAlmostEqual(m4.u, 0.25)
        self.assertAlmostEqual(m4.d, 0.05)
        self.assertTrue(m4.is_arbitrage_free)
        self.assertAlmostEqual(m4.risk_neutral_prob, 0.25)
        self.assertAlmostEqual(m4.forward_price, 110)

    def test_risk_and_return(self) -> None:
        rr = OneStepMarket(100, 110, 80, 100, 60, 0.8)

        def ev(P: Portfolio, E: float, sd: float) -> None:
            ku, kd = rr.portfolio_return(P, State.UP), rr.portfolio_return(P, State.DOWN)
            self.assertAlmostEqual(rr.expect(ku, kd), E)
            self.assertAlmostEqual(rr.stddev(ku, kd), sd)

        ev(Portfolio(50, 60), 0.12, 0.08)
        ev(Portfolio(0, 100), 0.10, 0.0)
        ev(Portfolio(125, 0), 0.15, 0.20)
        H = rr.split(10000, 0.5)
        self.assertAlmostEqual(H.x, 62.5)
        self.assertAlmostEqual(H.y, 50)
        ev(H, 0.125, 0.10)

    def test_forwards(self) -> None:
        P = forward_arbitrage(34, 100, 112, 38.60)
        self.assertIsNotNone(P)
        self.assertAlmostEqual(P.value0(34, 100), 0)
        self.assertAlmostEqual(P.value1(30, 112, 38.60), 0.52)
        self.assertAlmostEqual(P.value1(50, 112, 38.60), 0.52)
        self.assertIsNone(forward_arbitrage(34, 100, 112, 38.08))
        self.assertAlmostEqual(implied_foreign_rate(1.6, 1.5, 0.05), 0.12)

    def test_options(self) -> None:
        o = OneStepMarket(100, 110, 100, 120, 80, 0.5)
        self.assertAlmostEqual(o.price(20, 0), 150 / 11)   # call X=100
        self.assertAlmostEqual(o.price(30, 0), 225 / 11)   # Exercise 1.7a
        self.assertAlmostEqual(o.price(10, 0), 75 / 11)    # Exercise 1.7b
        self.assertAlmostEqual(OneStepMarket(100, 105, 100, 120, 80, 0.5).price(20, 0), 50 - 4000 / 105)
        self.assertAlmostEqual(OneStepMarket(100, 115, 100, 120, 80, 0.5).price(20, 0), 50 - 4000 / 115)
        self.assertAlmostEqual(o.price(0, 20), 50 / 11)    # Exercise 1.9

    def test_hedging(self) -> None:
        h = OneStepMarket(100, 110, 100, 160, 40, 0.5)
        self.assertAlmostEqual(h.price(60, 0), 350 / 11)
        self.assertAlmostEqual(hedged_purchase_cost(h, 100, 1, State.UP), 135)
        self.assertAlmostEqual(hedged_purchase_cost(h, 100, 1, State.DOWN), 75)
        self.assertAlmostEqual(hedged_purchase_cost(h, 100, 2, State.UP), 110)
        self.assertAlmostEqual(hedged_purchase_cost(h, 100, 2, State.DOWN), 110)
        for p in (0.1, 0.25, 0.5, 0.75, 0.9):  # Exercise 1.12
            hp = OneStepMarket(100, 110, 100, 160, 40, p)
            s1 = hp.stddev(hedged_purchase_cost(hp, 100, 1, State.UP), hedged_purchase_cost(hp, 100, 1, State.DOWN))
            self.assertAlmostEqual(s1, hp.stddev(160, 40) / 2)

    def test_arbitrage(self) -> None:
        self.assertIsNone(self.m.arbitrage())
        for S0 in (55.0, 36.0):
            bad = OneStepMarket(100, 110, S0, 60, 40, 0.8)
            P = bad.arbitrage()
            self.assertIsNotNone(P)
            self.assertAlmostEqual(bad.value0(P), 0)
            vu, vd = bad.value1(P, State.UP), bad.value1(P, State.DOWN)
            self.assertTrue(vu >= 0 and vd >= 0 and vu + vd > 0)


if __name__ == "__main__":
    unittest.main()
