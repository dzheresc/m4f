"""Unit tests for chapter 11."""

import math
import unittest

import derivatives
import rate_tree

NUM_STEPS = 4
TAU = 1.0
RATES = rate_tree.rate_lattice(0.05, 0.01, -0.01, NUM_STEPS)
PROBS = rate_tree.constant_probs(0.5, NUM_STEPS)


class LibraryTest(unittest.TestCase):

    def test_lattice_and_money_market(self) -> None:
        self.assertEqual(len(RATES), 5)
        self.assertAlmostEqual(RATES[2][2], 0.07)
        self.assertAlmostEqual(RATES[2][0], 0.03)
        account = rate_tree.money_market(RATES, TAU, "ud")
        self.assertAlmostEqual(account[1], math.exp(0.05))
        self.assertAlmostEqual(account[2], math.exp(0.05 + 0.06))
        self.assertAlmostEqual(
            rate_tree.one_step_price(0.05, 0.5), math.exp(-0.025)
        )

    def test_bond_pricing(self) -> None:
        b1 = rate_tree.bond_lattice(RATES, PROBS, TAU, 1)
        self.assertAlmostEqual(b1[0][0], math.exp(-0.05))
        b2 = rate_tree.bond_lattice(RATES, PROBS, TAU, 2)
        self.assertAlmostEqual(
            b2[0][0],
            math.exp(-0.05) * (0.5 * math.exp(-0.06) + 0.5 * math.exp(-0.04)),
        )
        b0 = rate_tree.bond_prices(RATES, PROBS, TAU, NUM_STEPS)
        self.assertEqual(len(b0), 4)
        for m in range(1, NUM_STEPS + 1):
            self.assertAlmostEqual(
                b0[m - 1], rate_tree.bond_lattice(RATES, PROBS, TAU, m)[0][0]
            )
        self.assertTrue(all(a > b for a, b in zip(b0, b0[1:])))
        self.assertAlmostEqual(rate_tree.yield_rate(b1[0][0], 0, 1, TAU), 0.05)
        # Discounted bond prices are martingales under p*.
        b4 = rate_tree.bond_lattice(RATES, PROBS, TAU, NUM_STEPS)
        for n in range(NUM_STEPS):
            for k in range(n + 1):
                self.assertAlmostEqual(
                    b4[n][k] * math.exp(RATES[n][k] * TAU),
                    0.5 * b4[n + 1][k + 1] + 0.5 * b4[n + 1][k],
                )

    def test_implied_probs(self) -> None:
        for p in (0.5, 0.6, 0.35):
            bond = rate_tree.bond_lattice(
                RATES, rate_tree.constant_probs(p, NUM_STEPS), TAU, NUM_STEPS
            )
            for row in rate_tree.implied_probs(bond, RATES, TAU):
                for q in row:
                    self.assertAlmostEqual(q, p)
        b2 = rate_tree.bond_lattice(RATES, PROBS, TAU, 2)
        self.assertAlmostEqual(
            rate_tree.implied_prob(b2, RATES, TAU, 0, 0), 0.5
        )

    def test_options(self) -> None:
        b2 = rate_tree.bond_lattice(RATES, PROBS, TAU, 2)
        b4 = rate_tree.bond_lattice(RATES, PROBS, TAU, NUM_STEPS)
        call = derivatives.bond_option(
            RATES, PROBS, TAU, NUM_STEPS, 2, 0.9, True
        )
        put = derivatives.bond_option(
            RATES, PROBS, TAU, NUM_STEPS, 2, 0.9, False
        )
        # Put-call parity on the bond: C - P = B(0,4) - X B(0,2).
        self.assertAlmostEqual(
            call[0][0] - put[0][0], b4[0][0] - 0.9 * b2[0][0]
        )
        american_put = derivatives.bond_option(
            RATES, PROBS, TAU, NUM_STEPS, 2, 0.9, False, american=True
        )
        self.assertGreaterEqual(american_put[0][0], put[0][0])
        self.assertAlmostEqual(american_put[0][0], 0.9 - b4[0][0])
        # A zero-strike call is the bond itself.
        zero_strike = derivatives.bond_option(
            RATES, PROBS, TAU, NUM_STEPS, 2, 0.0, True
        )
        self.assertAlmostEqual(zero_strike[0][0], b4[0][0])

    def test_swap_cap_floor(self) -> None:
        b0 = rate_tree.bond_prices(RATES, PROBS, TAU, NUM_STEPS)
        swap_rate = derivatives.swap_rate(b0)
        self.assertAlmostEqual(
            derivatives.payer_swap(RATES, PROBS, TAU, NUM_STEPS, swap_rate)[0][
                0
            ],
            0.0,
        )
        # The floating leg alone is worth 1 - B(0,N).
        zero_fixed = derivatives.payer_swap(RATES, PROBS, TAU, NUM_STEPS, 0.0)
        self.assertAlmostEqual(zero_fixed[0][0], 1 - b0[-1])
        for strike in (0.03, swap_rate, 0.07):
            cap = derivatives.cap(RATES, PROBS, TAU, NUM_STEPS, strike)[0][0]
            floor = derivatives.floor(RATES, PROBS, TAU, NUM_STEPS, strike)[0][
                0
            ]
            swap = derivatives.payer_swap(RATES, PROBS, TAU, NUM_STEPS, strike)[
                0
            ][0]
            self.assertAlmostEqual(cap - floor, swap)
            self.assertGreaterEqual(cap, 0.0)
            self.assertGreaterEqual(floor, 0.0)
        caplets = (
            derivatives.caplet(RATES, PROBS, TAU, n, 0.05)
            for n in range(NUM_STEPS)
        )
        self.assertAlmostEqual(
            derivatives.plain_sum(caplets),
            derivatives.cap(RATES, PROBS, TAU, NUM_STEPS, 0.05)[0][0],
        )
        # First caplet: a known payment discounted one step.
        first = math.exp(-0.05) * max(
            derivatives.floating_rate(RATES, TAU, 0, 0) - 0.05, 0.0
        )
        self.assertAlmostEqual(
            derivatives.caplet(RATES, PROBS, TAU, 0, 0.05), first
        )

    def test_discount_back_with_payment(self) -> None:
        # Paying 1 at each of n+1 = 1..4 is worth the sum of bond prices.
        values = rate_tree.discount_back(
            RATES, PROBS, TAU, [0.0] * (NUM_STEPS + 1), payment=lambda n, k: 1.0
        )
        self.assertAlmostEqual(
            values[0][0],
            sum(rate_tree.bond_prices(RATES, PROBS, TAU, NUM_STEPS)),
        )


class AgainstReferenceTest(unittest.TestCase):
    """Forward-induction bond_prices and the rolling caplet against the lattice
    computations they replaced, with node-dependent probabilities."""

    def test_against_lattices(self) -> None:
        n_steps = 60
        rates = rate_tree.rate_lattice(0.05, 0.004, -0.003, n_steps)
        probs = [
            [0.3 + 0.4 * math.fmod(0.618 * (n * 7 + k * 3), 1.0)
             for k in range(n + 1)]
            for n in range(n_steps)
        ]  # fmt: skip
        b0 = rate_tree.bond_prices(rates, probs, 0.25, n_steps)
        for m in range(1, n_steps + 1):
            ref = rate_tree.bond_lattice(rates, probs, 0.25, m)[0][0]
            self.assertLessEqual(abs(b0[m - 1] - ref), 1e-14 * ref)
        for fixing in (0, 1, 7, 59):

            def payment(n: int, k: int, fixing: int = fixing) -> float:
                if n != fixing:
                    return 0.0
                return max(
                    derivatives.floating_rate(rates, 0.25, n, k) - 0.05, 0.0
                )

            ref = derivatives.price_payments(
                rates, probs, 0.25, fixing + 1, payment
            )[0][0]
            self.assertEqual(
                derivatives.caplet(rates, probs, 0.25, fixing, 0.05), ref
            )


if __name__ == "__main__":
    unittest.main()
