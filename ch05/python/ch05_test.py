"""Unit tests for chapter 5."""

import math
import unittest

import portfolio
import risk

MEANS = [0.10, 0.15, 0.20]


def unit(i: int) -> list[float]:
    return [1.0 if j == i else 0.0 for j in range(3)]


class LibraryTest(unittest.TestCase):

    def setUp(self) -> None:
        super().setUp()
        self.cov = portfolio.covariance_matrix(
            [0.15, 0.25, 0.30],
            [[1.0, 0.3, 0.1], [0.3, 1.0, 0.4], [0.1, 0.4, 1.0]],
        )

    def test_scenarios(self) -> None:
        p = [0.2, 0.5, 0.3]
        k1, k2 = [-0.10, 0.10, 0.20], [0.20, 0.05, -0.05]
        self.assertAlmostEqual(risk.mean(k1, p), 0.09)
        self.assertAlmostEqual(risk.covariance(k1, k2, p), -0.009)
        rho = risk.correlation(k1, k2, p)
        for w in (0.0, 0.3, 0.5, 1.2):
            k_v = [w * a + (1 - w) * b for a, b in zip(k1, k2)]
            self.assertAlmostEqual(
                risk.std_dev(k_v, p),
                risk.two_asset_std(
                    w, risk.std_dev(k1, p), risk.std_dev(k2, p), rho
                ),
            )
            self.assertAlmostEqual(
                risk.mean(k_v, p),
                risk.two_asset_mean(w, risk.mean(k1, p), risk.mean(k2, p)),
            )

    def test_two_assets(self) -> None:
        for rho in (-0.9, -0.5, 0.0, 0.3):
            w = risk.min_variance_weight(0.10, 0.20, rho)
            s = risk.two_asset_std(w, 0.10, 0.20, rho)
            for dw in (-0.01, 0.01):
                self.assertLess(s, risk.two_asset_std(w + dw, 0.10, 0.20, rho))
        self.assertAlmostEqual(risk.two_asset_std(2 / 3, 0.10, 0.20, -1.0), 0.0)

    def test_inverse(self) -> None:
        cov_inv = portfolio.inverse(self.cov)
        for i in range(3):
            row = portfolio.vec_mat(unit(i), self.cov)
            for j in range(3):
                column = [cov_inv[k][j] for k in range(3)]
                self.assertAlmostEqual(
                    portfolio.dot(row, column), 1.0 if i == j else 0.0
                )

    def test_solve_symmetric(self) -> None:
        """Cholesky solve against the explicit inverse.

        On a 40-asset positive definite matrix, and on an indefinite symmetric
        matrix, which takes the fallback path.
        """
        n = 40
        spd = [  # F F^T + D: three factors plus idiosyncratic variance.
            [
                (0.02 + 0.001 * i if i == j else 0.0)
                + sum(
                    0.01 * math.cos(f * (i + 1.0)) * math.cos(f * (j + 1.0))
                    for f in (1, 2, 3)
                )
                for j in range(n)
            ]
            for i in range(n)
        ]
        indefinite = [[1.0, 2.0, 0.0], [2.0, 1.0, 0.0], [0.0, 0.0, 3.0]]
        for c in (spd, indefinite):
            b = [1.0 + 0.1 * i for i in range(len(c))]
            expected = portfolio.vec_mat(b, portfolio.inverse(c))
            (x,) = portfolio.solve_symmetric(c, [b])
            for x_i, e_i in zip(x, expected):
                self.assertLess(abs(x_i - e_i) / max(1.0, abs(e_i)), 1e-9)

    def test_frontier(self) -> None:
        w_min = portfolio.min_variance_portfolio(self.cov)
        self.assertAlmostEqual(sum(w_min), 1)
        mu_min = portfolio.portfolio_mean(w_min, MEANS)
        self.assertAlmostEqual(
            portfolio.min_variance_line(MEANS, self.cov, mu_min)[0], w_min[0]
        )
        for mu in (0.12, 0.15, 0.25):
            w = portfolio.min_variance_line(MEANS, self.cov, mu)
            self.assertAlmostEqual(sum(w), 1)
            self.assertAlmostEqual(portfolio.portfolio_mean(w, MEANS), mu)
            self.assertGreaterEqual(
                portfolio.portfolio_std(w, self.cov),
                portfolio.portfolio_std(w_min, self.cov),
            )
            # Any other portfolio with the same mean is riskier: move along
            # (1, -2, 1).
            self.assertLess(
                portfolio.portfolio_std(w, self.cov),
                portfolio.portfolio_std(
                    [w[0] + 0.01, w[1] - 0.02, w[2] + 0.01], self.cov
                ),
            )

    def test_capm(self) -> None:
        w_market = portfolio.market_portfolio(MEANS, self.cov, 0.05)
        mu_m = portfolio.portfolio_mean(w_market, MEANS)
        sigma_m = portfolio.portfolio_std(w_market, self.cov)
        self.assertAlmostEqual(sum(w_market), 1)
        self.assertAlmostEqual(portfolio.beta(w_market, w_market, self.cov), 1)
        self.assertAlmostEqual(
            portfolio.capital_market_line(sigma_m, 0.05, mu_m, sigma_m), mu_m
        )
        # The market portfolio is on the frontier, and the CML is tangent
        # there.
        self.assertAlmostEqual(
            portfolio.portfolio_std(
                portfolio.min_variance_line(MEANS, self.cov, mu_m), self.cov
            ),
            sigma_m,
        )
        for i in range(3):
            self.assertAlmostEqual(
                portfolio.security_market_line(
                    portfolio.beta(unit(i), w_market, self.cov), 0.05, mu_m
                ),
                MEANS[i],
            )


if __name__ == "__main__":
    unittest.main()
