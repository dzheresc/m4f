import unittest

from portfolio import (beta, capital_market_line, covariance_matrix, dot, inverse, market_portfolio,
                       min_variance_line, min_variance_portfolio, portfolio_mean, portfolio_std,
                       security_market_line, vec_mat)
from risk import (correlation, covariance, mean, min_variance_weight, std_dev, two_asset_mean,
                  two_asset_std)

M = [0.10, 0.15, 0.20]
C = covariance_matrix([0.15, 0.25, 0.30], [[1.0, 0.3, 0.1], [0.3, 1.0, 0.4], [0.1, 0.4, 1.0]])


class LibraryTest(unittest.TestCase):
    def test_scenarios(self) -> None:
        p = [0.2, 0.5, 0.3]
        K1, K2 = [-0.10, 0.10, 0.20], [0.20, 0.05, -0.05]
        self.assertAlmostEqual(mean(K1, p), 0.09)
        self.assertAlmostEqual(covariance(K1, K2, p), -0.009)
        rho = correlation(K1, K2, p)
        for w in (0.0, 0.3, 0.5, 1.2):
            KV = [w * a + (1 - w) * b for a, b in zip(K1, K2)]
            self.assertAlmostEqual(std_dev(KV, p), two_asset_std(w, std_dev(K1, p), std_dev(K2, p), rho))
            self.assertAlmostEqual(mean(KV, p), two_asset_mean(w, mean(K1, p), mean(K2, p)))

    def test_two_assets(self) -> None:
        for rho in (-0.9, -0.5, 0.0, 0.3):
            w = min_variance_weight(0.10, 0.20, rho)
            s = two_asset_std(w, 0.10, 0.20, rho)
            for dw in (-0.01, 0.01):
                self.assertLess(s, two_asset_std(w + dw, 0.10, 0.20, rho))
        self.assertAlmostEqual(two_asset_std(2 / 3, 0.10, 0.20, -1.0), 0.0)

    def test_inverse(self) -> None:
        Ci = inverse(C)
        for i in range(3):
            row = vec_mat([1.0 if j == i else 0.0 for j in range(3)], C)
            for j in range(3):
                self.assertAlmostEqual(dot(row, [Ci[k][j] for k in range(3)]), 1.0 if i == j else 0.0)

    def test_frontier(self) -> None:
        wmv = min_variance_portfolio(C)
        self.assertAlmostEqual(sum(wmv), 1)
        self.assertAlmostEqual(min_variance_line(M, C, portfolio_mean(wmv, M))[0], wmv[0])
        for mu in (0.12, 0.15, 0.25):
            w = min_variance_line(M, C, mu)
            self.assertAlmostEqual(sum(w), 1)
            self.assertAlmostEqual(portfolio_mean(w, M), mu)
            self.assertGreaterEqual(portfolio_std(w, C), portfolio_std(wmv, C))
            # any other portfolio with the same mean is riskier: move along (1, -2, 1)
            self.assertLess(portfolio_std(w, C), portfolio_std([w[0] + 0.01, w[1] - 0.02, w[2] + 0.01], C))

    def test_capm(self) -> None:
        wM = market_portfolio(M, C, 0.05)
        muM, sM = portfolio_mean(wM, M), portfolio_std(wM, C)
        self.assertAlmostEqual(sum(wM), 1)
        self.assertAlmostEqual(beta(wM, wM, C), 1)
        self.assertAlmostEqual(capital_market_line(sM, 0.05, muM, sM), muM)
        # the market portfolio is on the frontier, and the CML is tangent there
        self.assertAlmostEqual(portfolio_std(min_variance_line(M, C, muM), C), sM)
        for i in range(3):
            e = [1.0 if j == i else 0.0 for j in range(3)]
            self.assertAlmostEqual(security_market_line(beta(e, wM, C), 0.05, muM), M[i])


if __name__ == "__main__":
    unittest.main()
