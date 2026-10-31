"""Unit tests for chapter 3."""

import math
import unittest

import binomial_tree
import other_models
import returns


class LibraryTest(unittest.TestCase):

    def test_report_values(self) -> None:
        tree = binomial_tree.BinomialTree(100, 0.10, -0.05, 0.03)
        self.assertAlmostEqual(tree.expected_price(3, 0.6), 112.4864, places=9)
        self.assertAlmostEqual(tree.risk_neutral_prob(), 8 / 15)
        model = other_models.TrinomialModel(0.10, 0.0, -0.10, 0.02)
        self.assertAlmostEqual(model.max_middle_prob(), 0.8)
        self.assertAlmostEqual(
            other_models.lognormal_expected_price(100, 0.05, 0.2, 1),
            100 * math.exp(0.07),
        )

    def test_returns(self) -> None:
        path = [45.0, 48.0, 47.5, 50.0]
        self.assertAlmostEqual(
            returns.compound_return(returns.returns(path)),
            returns.one_step_return(45, 50),
        )
        self.assertAlmostEqual(
            sum(returns.log_returns(path)), returns.log_return(45, 50)
        )
        self.assertAlmostEqual(returns.expectation([1, 3], [0.5, 0.5]), 2)
        self.assertAlmostEqual(returns.variance([1, 3], [0.5, 0.5]), 1)

    def test_binomial(self) -> None:
        self.assertAlmostEqual(
            sum(binomial_tree.binomial_prob(5, k, 0.3) for k in range(6)), 1
        )
        # Large n: C(2000, 1000) alone overflows a float.
        self.assertAlmostEqual(
            sum(binomial_tree.binomial_prob(2000, k, 0.6) for k in range(2001)),
            1,
        )
        self.assertAlmostEqual(
            binomial_tree.binomial_prob(2000, 1000, 0.5), 0.017839011, places=9
        )
        long_tree = binomial_tree.BinomialTree(100, 0.02, -0.01, 0.005)
        self.assertAlmostEqual(  # E(S(n)) = S(0) (1 + E K)^n.
            long_tree.expected_price(2000, 0.6)
            / (100 * (1 + 0.6 * 0.02 + 0.4 * -0.01) ** 2000),
            1,
        )
        self.assertAlmostEqual(  # Against the node-by-node sum.
            long_tree.expected_price(50, 0.6),
            sum(
                long_tree.price(50, k) * binomial_tree.binomial_prob(50, k, 0.6)
                for k in range(51)
            ),
        )
        tree = binomial_tree.BinomialTree(60, 0.2, -0.1, 0.05)
        q = tree.risk_neutral_prob()
        self.assertTrue(tree.is_arbitrage_free())
        self.assertAlmostEqual(tree.expected_return(q), 0.05)
        for n in range(4):
            self.assertAlmostEqual(tree.expected_price(n, q), 60 * 1.05**n)
            self.assertAlmostEqual(
                tree.expected_price(n, 0.4),
                60 * (1 + tree.expected_return(0.4)) ** n,
            )
            for k in range(n + 1):
                self.assertAlmostEqual(
                    tree.conditional_discounted(n, k),
                    tree.discounted_price(n, k),
                )
        self.assertFalse(
            binomial_tree.BinomialTree(60, 0.2, 0.1, 0.05).is_arbitrage_free()
        )

    def test_trinomial(self) -> None:
        model = other_models.TrinomialModel(0.10, 0.0, -0.10, 0.02)
        for p_mid in (0.1, 0.5, 0.79):
            p_up, _, p_down = model.risk_neutral_probs(p_mid)
            self.assertAlmostEqual(p_up + p_mid + p_down, 1)
            self.assertAlmostEqual(p_up * 0.10 + p_down * -0.10, 0.02)
            self.assertGreater(min(p_up, p_down), 0)
        self.assertLess(model.risk_neutral_probs(0.81)[2], 0)

    def test_continuous_limit(self) -> None:
        self.assertAlmostEqual(other_models.norm_cdf(0), 0.5, places=7)
        self.assertAlmostEqual(other_models.norm_cdf(1.96), 0.9750021, places=6)
        self.assertAlmostEqual(
            other_models.norm_cdf(-1) + other_models.norm_cdf(1), 1
        )
        self.assertAlmostEqual(
            other_models.binomial_expected_price(100, 0.05, 0.2, 1, 100000),
            other_models.lognormal_expected_price(100, 0.05, 0.2, 1),
            places=4,
        )
        self.assertAlmostEqual(
            other_models.binomial_prob_above(100, 0.05, 0.2, 1, 20000, 110),
            other_models.lognormal_prob_above(100, 0.05, 0.2, 1, 110),
            places=2,
        )
        self.assertAlmostEqual(
            other_models.binomial_prob_above(100, 0.0, 0.2, 1, 7, 1e-9), 1
        )


if __name__ == "__main__":
    unittest.main()
