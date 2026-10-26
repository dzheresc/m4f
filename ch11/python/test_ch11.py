import math
import unittest

from derivatives import plain_sum, bond_option, cap, caplet, floating_rate, floor, payer_swap, swap_rate
from rate_tree import (bond_lattice, bond_prices, constant_probs, discount_back, implied_prob, implied_probs,
                       money_market, one_step_price, rate_lattice, yield_rate)

N, tau = 4, 1.0
rates = rate_lattice(0.05, 0.01, -0.01, N)
probs = constant_probs(0.5, N)


class LibraryTest(unittest.TestCase):
    def test_lattice_and_money_market(self) -> None:
        self.assertEqual(len(rates), 5)
        self.assertAlmostEqual(rates[2][2], 0.07)
        self.assertAlmostEqual(rates[2][0], 0.03)
        A = money_market(rates, tau, "ud")
        self.assertAlmostEqual(A[1], math.exp(0.05))
        self.assertAlmostEqual(A[2], math.exp(0.05 + 0.06))
        self.assertAlmostEqual(one_step_price(0.05, 0.5), math.exp(-0.025))

    def test_bond_pricing(self) -> None:
        B1 = bond_lattice(rates, probs, tau, 1)
        self.assertAlmostEqual(B1[0][0], math.exp(-0.05))
        B2 = bond_lattice(rates, probs, tau, 2)
        self.assertAlmostEqual(B2[0][0], math.exp(-0.05) * (0.5 * math.exp(-0.06) + 0.5 * math.exp(-0.04)))
        B0 = bond_prices(rates, probs, tau, N)
        self.assertEqual(len(B0), 4)
        for m in range(1, N + 1):
            self.assertAlmostEqual(B0[m - 1], bond_lattice(rates, probs, tau, m)[0][0])
        self.assertTrue(all(a > b for a, b in zip(B0, B0[1:])))
        self.assertAlmostEqual(yield_rate(B1[0][0], 0, 1, tau), 0.05)
        # discounted bond prices are martingales under p*
        B4 = bond_lattice(rates, probs, tau, N)
        for n in range(N):
            for k in range(n + 1):
                self.assertAlmostEqual(B4[n][k] * math.exp(rates[n][k] * tau), 0.5 * B4[n + 1][k + 1] + 0.5 * B4[n + 1][k])

    def test_implied_probs(self) -> None:
        for p in (0.5, 0.6, 0.35):
            B = bond_lattice(rates, constant_probs(p, N), tau, N)
            for row in implied_probs(B, rates, tau):
                for q in row:
                    self.assertAlmostEqual(q, p)
        self.assertAlmostEqual(implied_prob(bond_lattice(rates, probs, tau, 2), rates, tau, 0, 0), 0.5)

    def test_options(self) -> None:
        B4 = bond_lattice(rates, probs, tau, N)
        C = bond_option(rates, probs, tau, N, 2, 0.9, True)
        P = bond_option(rates, probs, tau, N, 2, 0.9, False)
        # put-call parity on the bond: C - P = B(0,4) - X B(0,2)
        self.assertAlmostEqual(C[0][0] - P[0][0], B4[0][0] - 0.9 * bond_lattice(rates, probs, tau, 2)[0][0])
        PA = bond_option(rates, probs, tau, N, 2, 0.9, False, american=True)
        self.assertGreaterEqual(PA[0][0], P[0][0])
        self.assertAlmostEqual(PA[0][0], 0.9 - B4[0][0])
        # zero strike call = the bond itself
        self.assertAlmostEqual(bond_option(rates, probs, tau, N, 2, 0.0, True)[0][0], B4[0][0])

    def test_swap_cap_floor(self) -> None:
        B0 = bond_prices(rates, probs, tau, N)
        K = swap_rate(B0)
        self.assertAlmostEqual(payer_swap(rates, probs, tau, N, K)[0][0], 0.0)
        # the floating leg alone is worth 1 - B(0,N)
        zero_fixed = payer_swap(rates, probs, tau, N, 0.0)[0][0]
        self.assertAlmostEqual(zero_fixed, 1 - B0[-1])
        for Kx in (0.03, K, 0.07):
            c, fo, s = cap(rates, probs, tau, N, Kx)[0][0], floor(rates, probs, tau, N, Kx)[0][0], payer_swap(rates, probs, tau, N, Kx)[0][0]
            self.assertAlmostEqual(c - fo, s)
            self.assertGreaterEqual(c, 0.0)
            self.assertGreaterEqual(fo, 0.0)
        self.assertAlmostEqual(plain_sum(caplet(rates, probs, tau, n, 0.05) for n in range(N)), cap(rates, probs, tau, N, 0.05)[0][0])
        # first caplet: known payment discounted one step
        self.assertAlmostEqual(caplet(rates, probs, tau, 0, 0.05), math.exp(-0.05) * max(floating_rate(rates, tau, 0, 0) - 0.05, 0.0))

    def test_discount_back_with_payment(self) -> None:
        # paying 1 at each of n+1 = 1..4 is worth the sum of bond prices
        V = discount_back(rates, probs, tau, [0.0] * (N + 1), payment=lambda n, k: 1.0)
        self.assertAlmostEqual(V[0][0], sum(bond_prices(rates, probs, tau, N)))


if __name__ == "__main__":
    unittest.main()
