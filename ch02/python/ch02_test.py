"""Unit tests for chapter 2."""

import unittest

import interest
import money_market


class LibraryTest(unittest.TestCase):

    def test_report_values(self) -> None:
        self.assertAlmostEqual(
            interest.annuity_present_value(1000, 0.10, 10),
            6144.567105,
            places=5,
        )
        self.assertAlmostEqual(
            interest.loan_installment(100000, 0.005, 300), 644.301401, places=5
        )
        self.assertAlmostEqual(
            interest.continuous_from_periodic(0.12, 12), 0.119404, places=5
        )
        self.assertAlmostEqual(
            money_market.money_market_rollover(1, [0.5, 0.5]), 4
        )

    def test_interest_identities(self) -> None:
        self.assertAlmostEqual(
            interest.simple_present_value(
                interest.simple_value(100, 0.1, 0.5), 0.1, 0.5
            ),
            100,
        )
        self.assertAlmostEqual(
            interest.simple_rate(100, interest.simple_value(100, 0.08, 2), 2),
            0.08,
        )
        self.assertAlmostEqual(interest.effective_rate(0.1, 1), 0.1)
        self.assertAlmostEqual(
            interest.periodic_from_continuous(
                interest.continuous_from_periodic(0.1, 12), 12
            ),
            0.1,
        )
        self.assertAlmostEqual(
            interest.periodic_from_periodic(
                interest.periodic_from_periodic(0.1, 4, 12), 12, 4
            ),
            0.1,
        )
        self.assertAlmostEqual(
            interest.continuous_effective_rate(
                interest.continuous_from_periodic(0.1, 1)
            ),
            0.1,
        )
        self.assertAlmostEqual(
            interest.periodic_value(100, 0.1, 1000000, 1),
            interest.continuous_value(100, 0.1, 1),
            places=4,
        )
        self.assertAlmostEqual(
            interest.annuity_present_value(
                interest.loan_installment(1000, 0.1, 5), 0.1, 5
            ),
            1000,
        )
        self.assertAlmostEqual(
            interest.annuity_future_value(1, 0.1, 5),
            interest.annuity_factor(0.1, 5) * 1.1**5,
        )

    def test_bond_identities(self) -> None:
        for m in (1, 2, 12, None):
            price = money_market.zero_coupon_price(0.08, 2.5, m)
            self.assertAlmostEqual(
                money_market.implied_rate(price, 2.5, m), 0.08
            )
        price = money_market.coupon_bond_price(100, 10, 3, 0.12)
        self.assertAlmostEqual(
            money_market.yield_to_maturity(price, 100, 10, 3), 0.12, places=8
        )
        self.assertAlmostEqual(
            money_market.coupon_bond_price(100, 10, 3, 0.10), 100
        )

    def test_closed_form_and_newton(self) -> None:
        """Closed-form price and Newton solver against a direct sum.

        Includes a zero and a tiny yield, and a price out of range.
        """
        for n in (1, 3, 60, 360):
            for y in (-0.5, -1e-9, 0.0, 1e-9, 0.004, 0.12, 2.0):
                direct = 100 * (1 + y) ** -n
                for k in range(1, n + 1):
                    direct += 5 * (1 + y) ** -k
                self.assertAlmostEqual(
                    money_market.coupon_bond_price(100, 5, n, y),
                    direct,
                    delta=1e-9 * direct,
                )
                self.assertAlmostEqual(
                    money_market.yield_to_maturity(direct, 100, 5, n),
                    y,
                    delta=1e-9,
                )
        self.assertAlmostEqual(
            money_market.yield_to_maturity(1e9, 100, 5, 3), -0.99, delta=1e-9
        )


if __name__ == "__main__":
    unittest.main()
