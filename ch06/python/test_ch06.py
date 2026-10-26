import math
import unittest

from forwards import (forward_arbitrage_profit, forward_price, forward_price_dividends,
                      forward_price_yield, long_forward_value)
from futures import futures_tree, hedged_std, margin_account, min_variance_hedge_ratio


class LibraryTest(unittest.TestCase):
    def test_forward_prices(self) -> None:
        self.assertAlmostEqual(forward_price(100, 0.08, 0.75), 100 * math.exp(0.06))
        self.assertAlmostEqual(forward_price_dividends(100, 0.08, 0.75, []), forward_price(100, 0.08, 0.75))
        self.assertAlmostEqual(forward_price_yield(100, 0.08, 0.0, 0.75), forward_price(100, 0.08, 0.75))
        F = forward_price_dividends(100, 0.08, 0.75, [(2, 0.25), (2, 0.5)])
        self.assertAlmostEqual(F, forward_price(100, 0.08, 0.75) - 2 * math.exp(0.04) - 2 * math.exp(0.02))
        self.assertAlmostEqual(forward_arbitrage_profit(forward_price(100, 0.08, 0.75), 100, 0.08, 0.75), 0)

    def test_forward_value(self) -> None:
        F0 = forward_price(100, 0.08, 0.75)
        self.assertAlmostEqual(long_forward_value(F0, F0, 0.08, 0.25), 0)
        # long forward = long stock + short bond paying F0 at T
        St = 104.0
        self.assertAlmostEqual(long_forward_value(F0, forward_price(St, 0.08, 0.25), 0.08, 0.25),
                               St - F0 * math.exp(-0.08 * 0.25))

    def test_futures_tree(self) -> None:
        tree = futures_tree(100, 0.10, -0.05, 0.03, 3)
        for n in range(4):
            for k in range(n + 1):
                self.assertAlmostEqual(tree[n][k], 100 * 1.1 ** k * 0.95 ** (n - k) * 1.03 ** (3 - n))

    def test_margin(self) -> None:
        prices = [106.0, 104.5, 101.0, 103.5, 99.0, 102.0]
        rows = margin_account(prices, 100, 1000, 750)
        self.assertEqual([c for _, c in rows], [0.0, 500.0, 0.0, 0.0, 0.0])
        # final balance = initial + calls + total gain
        self.assertAlmostEqual(rows[-1][0], 1000 + 500 + 100 * (prices[-1] - prices[0]))

    def test_hedge(self) -> None:
        dS, df, p = [-6.0, -1.0, 2.0, 5.0], [-5.0, -2.0, 3.0, 4.0], [0.25] * 4
        h = min_variance_hedge_ratio(dS, df, p)
        self.assertAlmostEqual(h, 29 / 27)
        for dh in (-0.05, 0.05):
            self.assertLess(hedged_std(h, dS, df, p), hedged_std(h + dh, dS, df, p))
        self.assertAlmostEqual(hedged_std(1.0, dS, dS, p), 0)


if __name__ == "__main__":
    unittest.main()
