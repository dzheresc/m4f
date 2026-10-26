import math
import unittest

from binomial import crr_price
from options import (american_parity_bounds, bound_violation, butterfly_gap, call_payoff,
                     european_call_bounds, european_put_bounds, parity_gap, profit, put_payoff)


def price(S0, X, T, is_call, american=False):
    return crr_price(S0, X, 0.05, 0.2, T, 200, is_call, american)


class LibraryTest(unittest.TestCase):
    def test_payoffs(self) -> None:
        for S in (80.0, 100.0, 120.0):
            self.assertAlmostEqual(call_payoff(S, 100) - put_payoff(S, 100), S - 100)
        self.assertAlmostEqual(profit(0.0, 10.0, 0.05, 1.0), -10 * math.exp(0.05))

    def test_parity(self) -> None:
        C, P = price(100, 100, 1, True), price(100, 100, 1, False)
        self.assertAlmostEqual(parity_gap(C, P, 100, 100, 0.05, 1), 0, places=9)
        self.assertAlmostEqual(parity_gap(12.0, 5.0, 100, 100, 0.05, 1), 7 - 100 + 100 * math.exp(-0.05))

    def test_bounds(self) -> None:
        for S0 in (80.0, 100.0, 120.0):
            for T in (0.25, 1.0):
                self.assertEqual(bound_violation(price(S0, 100, T, True), european_call_bounds(S0, 100, 0.05, T)), 0)
                self.assertEqual(bound_violation(price(S0, 100, T, False), european_put_bounds(S0, 100, 0.05, T)), 0)
                lo, hi = american_parity_bounds(S0, 100, 0.05, T)
                d = price(S0, 100, T, True, True) - price(S0, 100, T, False, True)
                self.assertTrue(lo <= d <= hi)
        self.assertAlmostEqual(bound_violation(3.5, (4.0, 100.0)), 0.5)
        self.assertAlmostEqual(bound_violation(101.0, (4.0, 100.0)), 1.0)

    def test_american(self) -> None:
        for S0 in (80.0, 100.0, 120.0):
            self.assertAlmostEqual(price(S0, 100, 1, True, True), price(S0, 100, 1, True), places=9)
            self.assertGreaterEqual(price(S0, 100, 1, False, True), price(S0, 100, 1, False))
            self.assertGreaterEqual(price(S0, 100, 1, False, True), put_payoff(S0, 100))
        Ps = [price(100, 100, T, False, True) for T in (0.25, 0.5, 1.0, 2.0)]
        self.assertEqual(Ps, sorted(Ps))

    def test_strike_dependence(self) -> None:
        Cs = [price(100, X, 1, True) for X in (90.0, 100.0, 110.0)]
        self.assertTrue(Cs[0] > Cs[1] > Cs[2])
        self.assertLessEqual(butterfly_gap(*Cs, 90.0, 100.0, 110.0), 0)
        self.assertLessEqual(Cs[0] - Cs[1], 10 * math.exp(-0.05))
        self.assertAlmostEqual(butterfly_gap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0), 1.5)


if __name__ == "__main__":
    unittest.main()
