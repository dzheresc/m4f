"""Unit tests for chapter 7."""

import math
import unittest

import binomial
import options


def price(
    s0: float,
    strike: float,
    maturity: float,
    is_call: bool,
    american: bool = False,
) -> float:
    return binomial.crr_price(
        s0, strike, 0.05, 0.2, maturity, 200, is_call, american
    )


class LibraryTest(unittest.TestCase):

    def test_payoffs(self) -> None:
        for s in (80.0, 100.0, 120.0):
            self.assertAlmostEqual(
                options.call_payoff(s, 100) - options.put_payoff(s, 100),
                s - 100,
            )
        self.assertAlmostEqual(
            options.profit(0.0, 10.0, 0.05, 1.0), -10 * math.exp(0.05)
        )

    def test_parity(self) -> None:
        call, put = price(100, 100, 1, True), price(100, 100, 1, False)
        self.assertAlmostEqual(
            options.parity_gap(call, put, 100, 100, 0.05, 1), 0, places=9
        )
        self.assertAlmostEqual(
            options.parity_gap(12.0, 5.0, 100, 100, 0.05, 1),
            7 - 100 + 100 * math.exp(-0.05),
        )

    def test_bounds(self) -> None:
        for s0 in (80.0, 100.0, 120.0):
            for maturity in (0.25, 1.0):
                self.assertEqual(
                    options.bound_violation(
                        price(s0, 100, maturity, True),
                        options.european_call_bounds(s0, 100, 0.05, maturity),
                    ),
                    0,
                )
                self.assertEqual(
                    options.bound_violation(
                        price(s0, 100, maturity, False),
                        options.european_put_bounds(s0, 100, 0.05, maturity),
                    ),
                    0,
                )
                lo, hi = options.american_parity_bounds(s0, 100, 0.05, maturity)
                d = price(s0, 100, maturity, True, True) - price(
                    s0, 100, maturity, False, True
                )
                self.assertTrue(lo <= d <= hi)
        self.assertAlmostEqual(options.bound_violation(3.5, (4.0, 100.0)), 0.5)
        self.assertAlmostEqual(
            options.bound_violation(101.0, (4.0, 100.0)), 1.0
        )

    def test_american(self) -> None:
        for s0 in (80.0, 100.0, 120.0):
            self.assertAlmostEqual(
                price(s0, 100, 1, True, True), price(s0, 100, 1, True), places=9
            )
            self.assertGreaterEqual(
                price(s0, 100, 1, False, True), price(s0, 100, 1, False)
            )
            self.assertGreaterEqual(
                price(s0, 100, 1, False, True), options.put_payoff(s0, 100)
            )
        american_puts = [
            price(100, 100, t, False, True) for t in (0.25, 0.5, 1.0, 2.0)
        ]
        self.assertEqual(american_puts, sorted(american_puts))

    def test_strike_dependence(self) -> None:
        calls = [price(100, x, 1, True) for x in (90.0, 100.0, 110.0)]
        self.assertTrue(calls[0] > calls[1] > calls[2])
        self.assertLessEqual(
            options.butterfly_gap(*calls, 90.0, 100.0, 110.0), 0
        )
        self.assertLessEqual(calls[0] - calls[1], 10 * math.exp(-0.05))
        self.assertAlmostEqual(
            options.butterfly_gap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0), 1.5
        )


if __name__ == "__main__":
    unittest.main()
