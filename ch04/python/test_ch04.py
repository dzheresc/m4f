import unittest

from no_arbitrage import (arbitrage_strategy, expected_discounted_value, is_arbitrage_free,
                          one_step_values, risk_neutral_probs)
from strategies import discounted_gains, gains, self_financing, self_financing_multi


class LibraryTest(unittest.TestCase):
    def test_self_financing(self) -> None:
        S = [100.0, 110.0, 104.5, 114.95]
        A = [100.0 * 1.03 ** n for n in range(4)]
        x = [5.0, 8.0, 3.0]
        y, V = self_financing(1000.0, S, A, x)
        self.assertAlmostEqual(V[3], 1079.2915, places=4)
        for n in range(1, 3):  # rebalancing at n costs exactly V(n)
            self.assertAlmostEqual(x[n] * S[n] + y[n] * A[n], V[n])
        self.assertAlmostEqual(gains(S, A, x, y), V[3] - V[0])
        self.assertAlmostEqual(discounted_gains(S, A, x), V[3] / A[3] - V[0] / A[0])

    def test_dividends(self) -> None:
        S = [[50.0, 55.0, 52.0, 58.0], [20.0, 19.0, 21.0, 22.0]]
        A = [1.05 ** n for n in range(4)]
        x = [[10.0, 12.0, 8.0], [20.0, 15.0, 25.0]]
        div = [[0.0] * 4, [0.0, 0.0, 2.0, 0.0]]
        _, V = self_financing_multi(1000.0, S, A, x, div)
        _, W = self_financing_multi(1000.0, S, A, x)
        self.assertAlmostEqual(V[3] - W[3], 2 * 15 * 1.05)

    def test_arbitrage(self) -> None:
        rets = [0.10, -0.05]
        self.assertTrue(is_arbitrage_free(rets, 0.03))
        self.assertIsNone(arbitrage_strategy(rets, 0.03, 100, 100))
        for r in (0.10, 0.12, -0.05, -0.08):
            x, y = arbitrage_strategy(rets, r, 100, 100)
            self.assertAlmostEqual(x * 100 + y * 100, 0)
            V1 = one_step_values(x, y, rets, r, 100, 100)
            self.assertGreaterEqual(min(V1), -1e-12)
            self.assertGreater(max(V1), 0)

    def test_ftap(self) -> None:
        for rets in ([0.10, -0.05], [-0.10, 0.0, 0.10], [0.3, -0.2, 0.05, 0.1]):
            for r in (-0.3, -0.09, 0.0, 0.04, 0.09, 0.5):
                q = risk_neutral_probs(rets, r)
                self.assertEqual(q is not None, is_arbitrage_free(rets, r))
                if q is not None:
                    self.assertAlmostEqual(sum(q), 1)
                    self.assertAlmostEqual(sum(a * b for a, b in zip(q, rets)), r)
                    self.assertGreater(min(q), 0)
        self.assertAlmostEqual(risk_neutral_probs([0.10, -0.05], 0.03)[0], 8 / 15)

    def test_martingale(self) -> None:
        q = 8 / 15
        rules = [lambda n, m: 1.0 + len(m) - sum(m), lambda n, m: 3.0 * sum(m) - 2.0, lambda n, m: float(n)]
        for rule in rules:
            for V0 in (0.0, 100.0):
                self.assertAlmostEqual(expected_discounted_value(100, 0.10, -0.05, 0.03, 4, V0, rule, q), V0)


if __name__ == "__main__":
    unittest.main()
