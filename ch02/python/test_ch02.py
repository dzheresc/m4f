import unittest

from interest import (annuity_factor, annuity_future_value, annuity_present_value,
                      continuous_effective_rate, continuous_from_periodic, continuous_value,
                      effective_rate, loan_installment, periodic_from_continuous,
                      periodic_from_periodic, periodic_value, simple_present_value, simple_rate,
                      simple_value)
from money_market import (coupon_bond_price, implied_rate, money_market_rollover, yield_to_maturity,
                          zero_coupon_price)


class LibraryTest(unittest.TestCase):
    def test_report_values(self) -> None:
        self.assertAlmostEqual(annuity_present_value(1000, 0.10, 10), 6144.567105, places=5)
        self.assertAlmostEqual(loan_installment(100000, 0.005, 300), 644.301401, places=5)
        self.assertAlmostEqual(continuous_from_periodic(0.12, 12), 0.119404, places=5)
        self.assertAlmostEqual(money_market_rollover(1, [0.5, 0.5]), 4)

    def test_identities(self) -> None:
        self.assertAlmostEqual(simple_present_value(simple_value(100, 0.1, 0.5), 0.1, 0.5), 100)
        self.assertAlmostEqual(simple_rate(100, simple_value(100, 0.08, 2), 2), 0.08)
        self.assertAlmostEqual(effective_rate(0.1, 1), 0.1)
        self.assertAlmostEqual(periodic_from_continuous(continuous_from_periodic(0.1, 12), 12), 0.1)
        self.assertAlmostEqual(periodic_from_periodic(periodic_from_periodic(0.1, 4, 12), 12, 4), 0.1)
        self.assertAlmostEqual(continuous_effective_rate(continuous_from_periodic(0.1, 1)), 0.1)
        self.assertAlmostEqual(periodic_value(100, 0.1, 1000000, 1), continuous_value(100, 0.1, 1), places=4)
        self.assertAlmostEqual(annuity_present_value(loan_installment(1000, 0.1, 5), 0.1, 5), 1000)
        self.assertAlmostEqual(annuity_future_value(1, 0.1, 5), annuity_factor(0.1, 5) * 1.1 ** 5)
        for m in (1, 2, 12, None):
            self.assertAlmostEqual(implied_rate(zero_coupon_price(0.08, 2.5, m), 2.5, m), 0.08)
        self.assertAlmostEqual(yield_to_maturity(coupon_bond_price(100, 10, 3, 0.12), 100, 10, 3), 0.12, places=8)
        self.assertAlmostEqual(coupon_bond_price(100, 10, 3, 0.10), 100)


if __name__ == "__main__":
    unittest.main()
