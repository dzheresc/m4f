package m4f.ch11

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.max
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class Ch11Test {
    private val numSteps = 4
    private val tau = 1.0
    private val rates = rateLattice(0.05, 0.01, -0.01, numSteps)
    private val probs = constantProbs(0.5, numSteps)

    private fun assertNear(expected: Double, actual: Double, eps: Double = 1e-9) =
        assertTrue(abs(expected - actual) < eps, "expected $expected, got $actual")

    @Test
    fun latticeAndMoneyMarket() {
        assertEquals(5, rates.size)
        assertNear(0.07, rates[2][2])
        assertNear(0.03, rates[2][0])
        val account = moneyMarket(rates, tau, "ud")
        assertNear(exp(0.05), account[1])
        assertNear(exp(0.05 + 0.06), account[2])
        assertNear(exp(-0.025), oneStepPrice(0.05, 0.5))
    }

    @Test
    fun bondPricing() {
        val b1 = bondLattice(rates, probs, tau, 1)
        assertNear(exp(-0.05), b1[0][0])
        val b2 = bondLattice(rates, probs, tau, 2)
        assertNear(exp(-0.05) * (0.5 * exp(-0.06) + 0.5 * exp(-0.04)), b2[0][0])
        val b0 = bondPrices(rates, probs, tau, numSteps)
        assertEquals(4, b0.size)
        for (m in 1..numSteps) assertNear(bondLattice(rates, probs, tau, m)[0][0], b0[m - 1])
        for (i in 1 until b0.size) assertTrue(b0[i - 1] > b0[i])
        assertNear(0.05, yieldRate(b1[0][0], 0, 1, tau))
        val b4 = bondLattice(rates, probs, tau, numSteps)
        for (n in 0 until numSteps) {
            for (k in 0..n) {
                val expected = 0.5 * b4[n + 1][k + 1] + 0.5 * b4[n + 1][k]
                assertNear(expected, b4[n][k] * exp(rates[n][k] * tau))
            }
        }
    }

    @Test
    fun impliedProbabilities() {
        for (p in listOf(0.5, 0.6, 0.35)) {
            val bond = bondLattice(rates, constantProbs(p, numSteps), tau, numSteps)
            for (row in impliedProbs(bond, rates, tau)) {
                for (q in row) assertNear(p, q)
            }
        }
        assertNear(0.5, impliedProb(bondLattice(rates, probs, tau, 2), rates, tau, 0, 0))
    }

    @Test
    fun options() {
        val b2 = bondLattice(rates, probs, tau, 2)
        val b4 = bondLattice(rates, probs, tau, numSteps)
        val call = bondOption(rates, probs, tau, numSteps, 2, 0.9, true)
        val put = bondOption(rates, probs, tau, numSteps, 2, 0.9, false)
        assertNear(b4[0][0] - 0.9 * b2[0][0], call[0][0] - put[0][0])
        val americanPut = bondOption(rates, probs, tau, numSteps, 2, 0.9, false, american = true)
        assertTrue(americanPut[0][0] >= put[0][0])
        assertNear(0.9 - b4[0][0], americanPut[0][0])
        assertNear(b4[0][0], bondOption(rates, probs, tau, numSteps, 2, 0.0, true)[0][0])
    }

    @Test
    fun swapCapFloor() {
        val b0 = bondPrices(rates, probs, tau, numSteps)
        val swapRate = swapRate(b0)
        assertNear(0.0, payerSwap(rates, probs, tau, numSteps, swapRate)[0][0])
        assertNear(1 - b0.last(), payerSwap(rates, probs, tau, numSteps, 0.0)[0][0])
        for (strike in listOf(0.03, swapRate, 0.07)) {
            val c = cap(rates, probs, tau, numSteps, strike)[0][0]
            val fl = floor(rates, probs, tau, numSteps, strike)[0][0]
            assertNear(payerSwap(rates, probs, tau, numSteps, strike)[0][0], c - fl)
            assertTrue(c >= 0.0 && fl >= 0.0)
        }
        val caplets = (0 until numSteps).map { caplet(rates, probs, tau, it, 0.05) }
        assertNear(cap(rates, probs, tau, numSteps, 0.05)[0][0], plainSum(caplets))
        val first = exp(-0.05) * max(floatingRate(rates, tau, 0, 0) - 0.05, 0.0)
        assertNear(first, caplet(rates, probs, tau, 0, 0.05))
        val annuity = discountBack(rates, probs, tau, List(numSteps + 1) { 0.0 }, { _, _ -> 1.0 })
        assertNear(plainSum(b0), annuity[0][0])
    }
}
