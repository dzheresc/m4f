import math
import unittest

from binomial_pricing import (binomial_sum, crr_call, early_exercise_nodes, european_price_sum,
                              one_step_replication, option_tree, replicating_position, stock_price)
from black_scholes import bs_call, bs_call_delta, bs_put, crr_tree_price, norm_cdf


def call(X):
    return lambda s: max(s - X, 0.0)


def put(X):
    return lambda s: max(X - s, 0.0)


class LibraryTest(unittest.TestCase):
    def test_one_step(self) -> None:
        x, y, C = one_step_replication(100, 0.2, -0.1, 0.1, 20, 0)
        self.assertAlmostEqual(x * 120 + y * 1.1, 20)
        self.assertAlmostEqual(x * 90 + y * 1.1, 0)
        self.assertAlmostEqual(C, 40 / 3.3)

    def test_replication_is_self_financing(self) -> None:
        S0, u, d, r = 100.0, 0.2, -0.1, 0.1
        V = option_tree(S0, u, d, r, 3, call(100))
        for n in range(3):
            for k in range(n + 1):
                x, y = replicating_position(S0, u, d, r, V, n, k)
                for kk in (k, k + 1):  # replicates both successors
                    self.assertAlmostEqual(x * stock_price(S0, u, d, n + 1, kk) + y * (1 + r) ** (n + 1), V[n + 1][kk])

    def test_sum_and_crr(self) -> None:
        for N in (1, 2, 5, 8):
            for X in (70.0, 100.0, 130.0):
                tree = option_tree(100, 0.2, -0.1, 0.1, N, call(X))[0][0]
                self.assertAlmostEqual(european_price_sum(100, 0.2, -0.1, 0.1, N, call(X)), tree)
                self.assertAlmostEqual(crr_call(100, X, 0.2, -0.1, 0.1, N)[0], tree)
                P = option_tree(100, 0.2, -0.1, 0.1, N, put(X))[0][0]
                self.assertAlmostEqual(tree - P, 100 - X * 1.1 ** -N)  # put-call parity
        self.assertAlmostEqual(binomial_sum(0.3, 6, 0), 1)

    def test_american(self) -> None:
        VA = option_tree(100, 0.1, -0.1, 0.02, 4, put(100), american=True)
        VE = option_tree(100, 0.1, -0.1, 0.02, 4, put(100))
        self.assertGreater(VA[0][0], VE[0][0])
        for n in range(5):
            for k in range(n + 1):
                self.assertGreaterEqual(VA[n][k], max(100 - stock_price(100, 0.1, -0.1, n, k), 0) - 1e-12)
        self.assertEqual(early_exercise_nodes(100, 0.1, -0.1, 0.02, 4, call(100)), [])
        self.assertAlmostEqual(option_tree(100, 0.1, -0.1, 0.02, 4, call(100), american=True)[0][0],
                               option_tree(100, 0.1, -0.1, 0.02, 4, call(100))[0][0])

    def test_black_scholes(self) -> None:
        self.assertAlmostEqual(norm_cdf(0), 0.5, places=14)
        self.assertAlmostEqual(norm_cdf(1.959963984540054), 0.975, places=12)
        self.assertAlmostEqual(norm_cdf(-8) + norm_cdf(8), 1, places=14)
        C, P = bs_call(100, 100, 0.05, 0.2, 1), bs_put(100, 100, 0.05, 0.2, 1)
        self.assertAlmostEqual(C, 10.450583572185565, places=10)
        self.assertAlmostEqual(C - P, 100 - 100 * math.exp(-0.05))
        h = 1e-4
        fd = (bs_call(100 + h, 100, 0.05, 0.2, 1) - bs_call(100 - h, 100, 0.05, 0.2, 1)) / (2 * h)
        self.assertAlmostEqual(bs_call_delta(100, 100, 0.05, 0.2, 1), fd, places=6)
        self.assertAlmostEqual(crr_tree_price(100, 100, 0.05, 0.2, 1, 2000, True), C, places=2)


if __name__ == "__main__":
    unittest.main()
