"""Unit tests for chapter 10."""

import math
import unittest

import bonds
import term_structure

MATURITIES = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0]
PRICES = [0.978, 0.953, 0.926, 0.898, 0.870, 0.842]


class LibraryTest(unittest.TestCase):

    def test_price_and_duration(self) -> None:
        z = bonds.zero_coupon(100.0, 4.0)
        self.assertAlmostEqual(bonds.price(z, 0.06), 100 * math.exp(-0.24))
        self.assertAlmostEqual(bonds.duration(z, 0.06), 4.0)
        self.assertAlmostEqual(bonds.duration(z, 0.06, 1.5), 2.5)
        b = bonds.coupon_bond(100.0, 5.0, 4)
        self.assertEqual([flow.t for flow in b], [1.0, 2.0, 3.0, 4.0])
        self.assertEqual([flow.amount for flow in b], [5.0, 5.0, 5.0, 105.0])
        h = 1e-5
        self.assertAlmostEqual(
            (bonds.price(b, 0.06 + h) - bonds.price(b, 0.06 - h)) / (2 * h),
            -bonds.duration(b, 0.06) * bonds.price(b, 0.06),
            places=4,
        )
        self.assertLess(bonds.duration(b, 0.06), 4.0)

    def test_investment_value(self) -> None:
        b = bonds.coupon_bond(100.0, 5.0, 4)
        # Unchanged yield: the investment grows at y from the price.
        for t in (1.0, 2.5, 4.0):
            self.assertAlmostEqual(
                bonds.investment_value(b, 0.06, t),
                bonds.price(b, 0.06) * math.exp(0.06 * t),
            )
            self.assertAlmostEqual(bonds.log_return(b, 0.06, 0.06, t), 0.06)
        # At the horizon t = D the value is immunised to first order.
        d = bonds.duration(b, 0.06)
        v0 = bonds.investment_value(b, 0.06, d)
        self.assertLess(abs(bonds.investment_value(b, 0.061, d) - v0), 1e-2)
        self.assertGreater(bonds.investment_value(b, 0.061, 1.0), 0)
        self.assertLess(
            bonds.investment_value(b, 0.07, 1.0),
            bonds.investment_value(b, 0.06, 1.0),
        )
        self.assertGreater(
            bonds.investment_value(b, 0.07, 4.0),
            bonds.investment_value(b, 0.06, 4.0),
        )

    def test_immunisation(self) -> None:
        bond_a = bonds.coupon_bond(100.0, 4.0, 5)
        bond_b = bonds.coupon_bond(100.0, 8.0, 10)
        d_a, d_b = bonds.duration(bond_a, 0.06), bonds.duration(bond_b, 0.06)
        w1, w2 = bonds.immunising_weights(d_a, d_b, 5.0)
        self.assertAlmostEqual(w1 + w2, 1.0)
        self.assertAlmostEqual(w1 * d_a + w2 * d_b, 5.0)
        p = bonds.immunise(1000.0, bond_a, bond_b, 0.06, 0.0, 5.0)
        self.assertAlmostEqual(
            bonds.portfolio_value(p, bond_a, bond_b, 0.06, 0.0), 1000.0
        )
        target = 1000.0 * math.exp(0.06 * 5)
        for y in (0.05, 0.07):
            v = p.q1 * bonds.investment_value(
                bond_a, y, 5.0
            ) + p.q2 * bonds.investment_value(bond_b, y, 5.0)
            # Convexity: the immunised portfolio never falls short.
            self.assertGreater(v, target - 1e-9)

    def test_dynamic_hedge(self) -> None:
        bond_a = bonds.coupon_bond(100.0, 4.0, 5)
        bond_b = bonds.coupon_bond(100.0, 8.0, 10)
        flat = [(0.5 * i, 0.06) for i in range(11)]
        steps = bonds.dynamic_hedge(1000.0, bond_a, bond_b, 5.0, flat)
        self.assertEqual(len(steps), 11)
        self.assertAlmostEqual(
            steps[-1].value, 1000.0 * math.exp(0.3), places=6
        )
        for st in steps[:-1]:
            self.assertAlmostEqual(st.duration, 5.0 - st.t, places=9)
        fixed = bonds.dynamic_hedge(
            1000.0, bond_a, bond_b, 5.0, flat, rebalance=False
        )
        self.assertAlmostEqual(
            fixed[-1].value, 1000.0 * math.exp(0.3), places=6
        )

    def test_term_structure(self) -> None:
        forwards = term_structure.forward_rates(MATURITIES, PRICES)
        self.assertAlmostEqual(
            forwards[0], term_structure.yield_rate(PRICES[0], 0.5)
        )
        rebuilt = term_structure.prices_from_forward_rates(MATURITIES, forwards)
        for b, back in zip(PRICES, rebuilt):
            self.assertAlmostEqual(b, back)
        self.assertAlmostEqual(
            term_structure.forward_rate(PRICES[1], 1.0, PRICES[5], 3.0),
            sum(forwards[2:6]) / 4,
        )
        self.assertAlmostEqual(
            term_structure.implied_price(PRICES[1], PRICES[3]),
            math.exp(
                -term_structure.forward_rate(PRICES[1], 1.0, PRICES[3], 2.0)
            ),
        )
        account = term_structure.money_market_account(MATURITIES, PRICES)
        for a, b in zip(account, PRICES):
            self.assertAlmostEqual(a, 1 / b)
        p = term_structure.coupon_bond_price(
            [1.0, 2.0, 3.0], [6.0, 6.0, 106.0], MATURITIES, PRICES
        )
        self.assertAlmostEqual(
            p, 6 * PRICES[1] + 6 * PRICES[3] + 106 * PRICES[5]
        )
        y = term_structure.yield_to_maturity(
            p, [1.0, 2.0, 3.0], [6.0, 6.0, 106.0]
        )
        self.assertAlmostEqual(
            6 * math.exp(-y) + 6 * math.exp(-2 * y) + 106 * math.exp(-3 * y),
            p,
            places=8,
        )
        self.assertAlmostEqual(
            term_structure.yield_to_maturity(PRICES[5] * 100, [3.0], [100.0]),
            term_structure.yield_rate(PRICES[5], 3.0),
            places=9,
        )


if __name__ == "__main__":
    unittest.main()
