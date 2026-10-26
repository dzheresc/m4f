import math
import unittest

from bonds import (coupon_bond, duration, dynamic_hedge, immunise, immunising_weights, investment_value, log_return,
                   price, value, zero_coupon)
from term_structure import (coupon_bond_price, forward_rate, forward_rates, implied_price, money_market_account,
                            prices_from_forward_rates, yield_rate, yield_to_maturity)

Ts = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0]
Bs = [0.978, 0.953, 0.926, 0.898, 0.870, 0.842]


class LibraryTest(unittest.TestCase):
    def test_price_and_duration(self) -> None:
        z = zero_coupon(100.0, 4.0)
        self.assertAlmostEqual(price(z, 0.06), 100 * math.exp(-0.24))
        self.assertAlmostEqual(duration(z, 0.06), 4.0)
        self.assertAlmostEqual(duration(z, 0.06, 1.5), 2.5)
        b = coupon_bond(100.0, 5.0, 4)
        self.assertEqual([fl.t for fl in b], [1.0, 2.0, 3.0, 4.0])
        self.assertEqual([fl.C for fl in b], [5.0, 5.0, 5.0, 105.0])
        h = 1e-5
        self.assertAlmostEqual((price(b, 0.06 + h) - price(b, 0.06 - h)) / (2 * h), -duration(b, 0.06) * price(b, 0.06), places=4)
        self.assertLess(duration(b, 0.06), 4.0)

    def test_investment_value(self) -> None:
        b = coupon_bond(100.0, 5.0, 4)
        # unchanged yield: the investment grows at y from the price
        for t in (1.0, 2.5, 4.0):
            self.assertAlmostEqual(investment_value(b, 0.06, t), price(b, 0.06) * math.exp(0.06 * t))
            self.assertAlmostEqual(log_return(b, 0.06, 0.06, t), 0.06)
        # at the horizon t = D the value is immunised to first order
        D = duration(b, 0.06)
        v0 = investment_value(b, 0.06, D)
        self.assertLess(abs(investment_value(b, 0.061, D) - v0), 1e-2)
        self.assertGreater(investment_value(b, 0.061, 1.0), 0)
        self.assertLess(investment_value(b, 0.07, 1.0), investment_value(b, 0.06, 1.0))
        self.assertGreater(investment_value(b, 0.07, 4.0), investment_value(b, 0.06, 4.0))

    def test_immunisation(self) -> None:
        A, B = coupon_bond(100.0, 4.0, 5), coupon_bond(100.0, 8.0, 10)
        w1, w2 = immunising_weights(duration(A, 0.06), duration(B, 0.06), 5.0)
        self.assertAlmostEqual(w1 + w2, 1.0)
        self.assertAlmostEqual(w1 * duration(A, 0.06) + w2 * duration(B, 0.06), 5.0)
        p = immunise(1000.0, A, B, 0.06, 0.0, 5.0)
        self.assertAlmostEqual(value(p, A, B, 0.06, 0.0), 1000.0)
        target = 1000.0 * math.exp(0.06 * 5)
        for y in (0.05, 0.07):
            v = p.q1 * investment_value(A, y, 5.0) + p.q2 * investment_value(B, y, 5.0)
            self.assertGreater(v, target - 1e-9)  # convexity: immunised portfolio never falls short

    def test_dynamic_hedge(self) -> None:
        A, B = coupon_bond(100.0, 4.0, 5), coupon_bond(100.0, 8.0, 10)
        flat = [(0.5 * i, 0.06) for i in range(11)]
        steps = dynamic_hedge(1000.0, A, B, 5.0, flat)
        self.assertEqual(len(steps), 11)
        self.assertAlmostEqual(steps[-1].value, 1000.0 * math.exp(0.3), places=6)
        for st in steps[:-1]:
            self.assertAlmostEqual(st.duration, 5.0 - st.t, places=9)
        static = dynamic_hedge(1000.0, A, B, 5.0, flat, rebalance=False)
        self.assertAlmostEqual(static[-1].value, 1000.0 * math.exp(0.3), places=6)

    def test_term_structure(self) -> None:
        fw = forward_rates(Ts, Bs)
        self.assertAlmostEqual(fw[0], yield_rate(Bs[0], 0.5))
        for T, B, back in zip(Ts, Bs, prices_from_forward_rates(Ts, fw)):
            self.assertAlmostEqual(B, back)
        self.assertAlmostEqual(forward_rate(Bs[1], 1.0, Bs[5], 3.0), sum(fw[2:6]) / 4)
        self.assertAlmostEqual(implied_price(Bs[1], Bs[3]), math.exp(-forward_rate(Bs[1], 1.0, Bs[3], 2.0)))
        A = money_market_account(Ts, Bs)
        for a, B in zip(A, Bs):
            self.assertAlmostEqual(a, 1 / B)
        P = coupon_bond_price([1.0, 2.0, 3.0], [6.0, 6.0, 106.0], Ts, Bs)
        self.assertAlmostEqual(P, 6 * Bs[1] + 6 * Bs[3] + 106 * Bs[5])
        y = yield_to_maturity(P, [1.0, 2.0, 3.0], [6.0, 6.0, 106.0])
        self.assertAlmostEqual(6 * math.exp(-y) + 6 * math.exp(-2 * y) + 106 * math.exp(-3 * y), P, places=8)
        self.assertAlmostEqual(yield_to_maturity(Bs[5] * 100, [3.0], [100.0]), yield_rate(Bs[5], 3.0), places=9)


if __name__ == "__main__":
    unittest.main()
