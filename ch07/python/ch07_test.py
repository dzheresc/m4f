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


def reference_crr(
    s0: float,
    strike: float,
    num_steps: int,
    is_call: bool,
    american: bool,
) -> float:
    """CRR backward induction pricing every node with two powers, as the book
    writes it (r = 0.05, sigma = 0.3, T = 1.5)."""
    tau = 1.5 / num_steps
    up = math.exp(0.3 * math.sqrt(tau))
    down = 1 / up
    growth = math.exp(0.05 * tau)
    q = (growth - down) / (up - down)

    def payoff(s: float) -> float:
        return max(s - strike, 0.0) if is_call else max(strike - s, 0.0)

    v = [
        payoff(s0 * up**k * down ** (num_steps - k))
        for k in range(num_steps + 1)
    ]
    for n in range(num_steps - 1, -1, -1):
        for k in range(n + 1):
            v[k] = (q * v[k + 1] + (1 - q) * v[k]) / growth
            if american:
                v[k] = max(v[k], payoff(s0 * up**k * down ** (n - k)))
    return v[0]


class LibraryTest(unittest.TestCase):

    def test_against_reference(self) -> None:
        for n in (1, 7, 200):
            for is_call in (True, False):
                for american in (False, True):
                    for strike in (80.0, 100.0, 125.0):
                        self.assertAlmostEqual(
                            binomial.crr_price(
                                100,
                                strike,
                                0.05,
                                0.3,
                                1.5,
                                n,
                                is_call,
                                american,
                            ),
                            reference_crr(100, strike, n, is_call, american),
                            places=9,
                        )

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
