"""Unit tests for chapter 6."""

import math
import unittest

import forwards
import futures


class LibraryTest(unittest.TestCase):

    def test_forward_prices(self) -> None:
        plain = forwards.forward_price(100, 0.08, 0.75)
        self.assertAlmostEqual(plain, 100 * math.exp(0.06))
        self.assertAlmostEqual(
            forwards.forward_price_dividends(100, 0.08, 0.75, []), plain
        )
        self.assertAlmostEqual(
            forwards.forward_price_yield(100, 0.08, 0.0, 0.75), plain
        )
        with_dividends = forwards.forward_price_dividends(
            100, 0.08, 0.75, [(2, 0.25), (2, 0.5)]
        )
        self.assertAlmostEqual(
            with_dividends, plain - 2 * math.exp(0.04) - 2 * math.exp(0.02)
        )
        self.assertAlmostEqual(
            forwards.forward_arbitrage_profit(plain, 100, 0.08, 0.75), 0
        )

    def test_forward_value(self) -> None:
        f0 = forwards.forward_price(100, 0.08, 0.75)
        self.assertAlmostEqual(
            forwards.long_forward_value(f0, f0, 0.08, 0.25), 0
        )
        # A long forward is a long stock plus a short bond paying f0 at T.
        st = 104.0
        ft = forwards.forward_price(st, 0.08, 0.25)
        self.assertAlmostEqual(
            forwards.long_forward_value(f0, ft, 0.08, 0.25),
            st - f0 * math.exp(-0.08 * 0.25),
        )

    def test_futures_tree(self) -> None:
        tree = futures.futures_tree(100, 0.10, -0.05, 0.03, 3)
        for n in range(4):
            for k in range(n + 1):
                self.assertAlmostEqual(
                    tree[n][k], 100 * 1.1**k * 0.95 ** (n - k) * 1.03 ** (3 - n)
                )

    def test_margin(self) -> None:
        prices = [106.0, 104.5, 101.0, 103.5, 99.0, 102.0]
        rows = futures.margin_account(prices, 100, 1000, 750)
        self.assertEqual([c for _, c in rows], [0.0, 500.0, 0.0, 0.0, 0.0])
        # Final balance = initial + calls + total gain.
        self.assertAlmostEqual(
            rows[-1][0], 1000 + 500 + 100 * (prices[-1] - prices[0])
        )

    def test_hedge(self) -> None:
        ds = [-6.0, -1.0, 2.0, 5.0]
        df = [-5.0, -2.0, 3.0, 4.0]
        p = [0.25] * 4
        h = futures.min_variance_hedge_ratio(ds, df, p)
        self.assertAlmostEqual(h, 29 / 27)
        for dh in (-0.05, 0.05):
            self.assertLess(
                futures.hedged_std(h, ds, df, p),
                futures.hedged_std(h + dh, ds, df, p),
            )
        self.assertAlmostEqual(futures.hedged_std(1.0, ds, ds, p), 0)


if __name__ == "__main__":
    unittest.main()
