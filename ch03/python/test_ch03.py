import math
import unittest

from binomial_tree import BinomialTree, binomial_prob
from other_models import (TrinomialModel, binomial_expected_price, binomial_prob_above,
                          lognormal_expected_price, lognormal_prob_above, norm_cdf)
from returns import (compound_return, expectation, log_return, log_returns, one_step_return,
                     returns, variance)


class LibraryTest(unittest.TestCase):
    def test_report_values(self) -> None:
        self.assertAlmostEqual(BinomialTree(100, 0.10, -0.05, 0.03).expected_price(3, 0.6), 112.4864, places=9)
        self.assertAlmostEqual(BinomialTree(100, 0.10, -0.05, 0.03).risk_neutral_prob(), 8 / 15)
        self.assertAlmostEqual(TrinomialModel(0.10, 0.0, -0.10, 0.02).max_middle_prob(), 0.8)
        self.assertAlmostEqual(lognormal_expected_price(100, 0.05, 0.2, 1), 100 * math.exp(0.07))

    def test_returns(self) -> None:
        path = [45.0, 48.0, 47.5, 50.0]
        self.assertAlmostEqual(compound_return(returns(path)), one_step_return(45, 50))
        self.assertAlmostEqual(sum(log_returns(path)), log_return(45, 50))
        self.assertAlmostEqual(expectation([1, 3], [0.5, 0.5]), 2)
        self.assertAlmostEqual(variance([1, 3], [0.5, 0.5]), 1)

    def test_binomial(self) -> None:
        self.assertAlmostEqual(sum(binomial_prob(5, k, 0.3) for k in range(6)), 1)
        tree = BinomialTree(60, 0.2, -0.1, 0.05)
        q = tree.risk_neutral_prob()
        self.assertTrue(tree.is_arbitrage_free())
        self.assertAlmostEqual(tree.expected_return(q), 0.05)
        for n in range(4):
            self.assertAlmostEqual(tree.expected_price(n, q), 60 * 1.05 ** n)
            self.assertAlmostEqual(tree.expected_price(n, 0.4), 60 * (1 + tree.expected_return(0.4)) ** n)
            for k in range(n + 1):
                self.assertAlmostEqual(tree.conditional_discounted(n, k), tree.discounted_price(n, k))
        self.assertFalse(BinomialTree(60, 0.2, 0.1, 0.05).is_arbitrage_free())

    def test_trinomial(self) -> None:
        tri = TrinomialModel(0.10, 0.0, -0.10, 0.02)
        for pm in (0.1, 0.5, 0.79):
            pu, _, pd = tri.risk_neutral_probs(pm)
            self.assertAlmostEqual(pu + pm + pd, 1)
            self.assertAlmostEqual(pu * 0.10 + pd * -0.10, 0.02)
            self.assertGreater(min(pu, pd), 0)
        self.assertLess(tri.risk_neutral_probs(0.81)[2], 0)

    def test_continuous_limit(self) -> None:
        self.assertAlmostEqual(norm_cdf(0), 0.5, places=7)
        self.assertAlmostEqual(norm_cdf(1.96), 0.9750021, places=6)
        self.assertAlmostEqual(norm_cdf(-1) + norm_cdf(1), 1)
        self.assertAlmostEqual(binomial_expected_price(100, 0.05, 0.2, 1, 100000),
                               lognormal_expected_price(100, 0.05, 0.2, 1), places=4)
        self.assertAlmostEqual(binomial_prob_above(100, 0.05, 0.2, 1, 20000, 110),
                               lognormal_prob_above(100, 0.05, 0.2, 1, 110), places=2)
        self.assertAlmostEqual(binomial_prob_above(100, 0.0, 0.2, 1, 7, 1e-9), 1)


if __name__ == "__main__":
    unittest.main()
