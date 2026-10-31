"""Unit tests for chapter 8."""

import math
import unittest

import binomial_pricing
import black_scholes


def call(strike: float) -> binomial_pricing.Payoff:
    return lambda s: max(s - strike, 0.0)


def put(strike: float) -> binomial_pricing.Payoff:
    return lambda s: max(strike - s, 0.0)


class LibraryTest(unittest.TestCase):

    def test_one_step(self) -> None:
        x, y, price = binomial_pricing.one_step_replication(
            100, 0.2, -0.1, 0.1, 20, 0
        )
        self.assertAlmostEqual(x * 120 + y * 1.1, 20)
        self.assertAlmostEqual(x * 90 + y * 1.1, 0)
        self.assertAlmostEqual(price, 40 / 3.3)

    def test_replication_is_self_financing(self) -> None:
        s0, u, d, r = 100.0, 0.2, -0.1, 0.1
        values = binomial_pricing.option_tree(s0, u, d, r, 3, call(100))
        for n in range(3):
            for k in range(n + 1):
                x, y = binomial_pricing.replicating_position(
                    s0, u, d, r, values, n, k
                )
                for next_k in (k, k + 1):  # Replicates both successors.
                    self.assertAlmostEqual(
                        x
                        * binomial_pricing.stock_price(s0, u, d, n + 1, next_k)
                        + y * (1 + r) ** (n + 1),
                        values[n + 1][next_k],
                    )

    def test_sum_and_crr(self) -> None:
        for num_steps in (1, 2, 5, 8):
            for strike in (70.0, 100.0, 130.0):
                tree = binomial_pricing.option_tree(
                    100, 0.2, -0.1, 0.1, num_steps, call(strike)
                )[0][0]
                self.assertAlmostEqual(
                    binomial_pricing.european_price_sum(
                        100, 0.2, -0.1, 0.1, num_steps, call(strike)
                    ),
                    tree,
                )
                self.assertAlmostEqual(
                    binomial_pricing.crr_call(
                        100, strike, 0.2, -0.1, 0.1, num_steps
                    )[0],
                    tree,
                )
                put_price = binomial_pricing.option_tree(
                    100, 0.2, -0.1, 0.1, num_steps, put(strike)
                )[0][0]
                # Put-call parity.
                self.assertAlmostEqual(
                    tree - put_price, 100 - strike * 1.1**-num_steps
                )
        self.assertAlmostEqual(binomial_pricing.binomial_sum(0.3, 6, 0), 1)

    def test_american(self) -> None:
        american = binomial_pricing.option_tree(
            100, 0.1, -0.1, 0.02, 4, put(100), american=True
        )
        european = binomial_pricing.option_tree(
            100, 0.1, -0.1, 0.02, 4, put(100)
        )
        self.assertGreater(american[0][0], european[0][0])
        for n in range(5):
            for k in range(n + 1):
                s = binomial_pricing.stock_price(100, 0.1, -0.1, n, k)
                self.assertGreaterEqual(american[n][k], max(100 - s, 0) - 1e-12)
        self.assertEqual(
            binomial_pricing.early_exercise_nodes(
                100, 0.1, -0.1, 0.02, 4, call(100)
            ),
            [],
        )
        self.assertAlmostEqual(
            binomial_pricing.option_tree(
                100, 0.1, -0.1, 0.02, 4, call(100), american=True
            )[0][0],
            binomial_pricing.option_tree(100, 0.1, -0.1, 0.02, 4, call(100))[0][
                0
            ],
        )

    def test_black_scholes(self) -> None:
        self.assertAlmostEqual(black_scholes.norm_cdf(0), 0.5, places=14)
        self.assertAlmostEqual(
            black_scholes.norm_cdf(1.959963984540054), 0.975, places=12
        )
        self.assertAlmostEqual(
            black_scholes.norm_cdf(-8) + black_scholes.norm_cdf(8), 1, places=14
        )
        call_price = black_scholes.bs_call(100, 100, 0.05, 0.2, 1)
        put_price = black_scholes.bs_put(100, 100, 0.05, 0.2, 1)
        self.assertAlmostEqual(call_price, 10.450583572185565, places=10)
        self.assertAlmostEqual(
            call_price - put_price, 100 - 100 * math.exp(-0.05)
        )
        h = 1e-4
        finite_diff = (
            black_scholes.bs_call(100 + h, 100, 0.05, 0.2, 1)
            - black_scholes.bs_call(100 - h, 100, 0.05, 0.2, 1)
        ) / (2 * h)
        self.assertAlmostEqual(
            black_scholes.bs_call_delta(100, 100, 0.05, 0.2, 1),
            finite_diff,
            places=6,
        )
        self.assertAlmostEqual(
            black_scholes.crr_tree_price(100, 100, 0.05, 0.2, 1, 2000, True),
            call_price,
            places=2,
        )


if __name__ == "__main__":
    unittest.main()
