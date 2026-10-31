"""Unit tests for chapter 4."""

import unittest

import no_arbitrage
import strategies


class LibraryTest(unittest.TestCase):

    def test_self_financing(self) -> None:
        s = [100.0, 110.0, 104.5, 114.95]
        a = [100.0 * 1.03**n for n in range(4)]
        x = [5.0, 8.0, 3.0]
        y, values = strategies.self_financing(1000.0, s, a, x)
        self.assertAlmostEqual(values[3], 1079.2915, places=4)
        for n in range(1, 3):  # Rebalancing at n costs exactly V(n).
            self.assertAlmostEqual(x[n] * s[n] + y[n] * a[n], values[n])
        self.assertAlmostEqual(
            strategies.gains(s, a, x, y), values[3] - values[0]
        )
        self.assertAlmostEqual(
            strategies.discounted_gains(s, a, x),
            values[3] / a[3] - values[0] / a[0],
        )

    def test_dividends(self) -> None:
        s = [[50.0, 55.0, 52.0, 58.0], [20.0, 19.0, 21.0, 22.0]]
        a = [1.05**n for n in range(4)]
        x = [[10.0, 12.0, 8.0], [20.0, 15.0, 25.0]]
        dividends = [[0.0] * 4, [0.0, 0.0, 2.0, 0.0]]
        _, with_div = strategies.self_financing_multi(
            1000.0, s, a, x, dividends
        )
        _, without = strategies.self_financing_multi(1000.0, s, a, x)
        self.assertAlmostEqual(with_div[3] - without[3], 2 * 15 * 1.05)

    def test_arbitrage(self) -> None:
        returns = [0.10, -0.05]
        self.assertTrue(no_arbitrage.is_arbitrage_free(returns, 0.03))
        self.assertIsNone(
            no_arbitrage.arbitrage_strategy(returns, 0.03, 100, 100)
        )
        for r in (0.10, 0.12, -0.05, -0.08):
            x, y = no_arbitrage.arbitrage_strategy(returns, r, 100, 100)
            self.assertAlmostEqual(x * 100 + y * 100, 0)
            v1 = no_arbitrage.one_step_values(x, y, returns, r, 100, 100)
            self.assertGreaterEqual(min(v1), -1e-12)
            self.assertGreater(max(v1), 0)

    def test_fundamental_theorem(self) -> None:
        for returns in (
            [0.10, -0.05],
            [-0.10, 0.0, 0.10],
            [0.3, -0.2, 0.05, 0.1],
        ):
            for r in (-0.3, -0.09, 0.0, 0.04, 0.09, 0.5):
                q = no_arbitrage.risk_neutral_probs(returns, r)
                self.assertEqual(
                    q is not None, no_arbitrage.is_arbitrage_free(returns, r)
                )
                if q is not None:
                    self.assertAlmostEqual(sum(q), 1)
                    self.assertAlmostEqual(
                        sum(a * b for a, b in zip(q, returns)), r
                    )
                    self.assertGreater(min(q), 0)
        self.assertAlmostEqual(
            no_arbitrage.risk_neutral_probs([0.10, -0.05], 0.03)[0], 8 / 15
        )

    def test_martingale(self) -> None:
        q = 8 / 15
        rules = [
            lambda n, m: 1.0 + len(m) - sum(m),
            lambda n, m: 3.0 * sum(m) - 2.0,
            lambda n, m: float(n),
        ]
        for rule in rules:
            for v0 in (0.0, 100.0):
                self.assertAlmostEqual(
                    no_arbitrage.expected_discounted_value(
                        100, 0.10, -0.05, 0.03, 4, v0, rule, q
                    ),
                    v0,
                )


if __name__ == "__main__":
    unittest.main()
