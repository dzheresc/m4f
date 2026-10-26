package m4f.ch11

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.max
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class Ch11Test {
    private fun near(a: Double, b: Double, eps: Double = 1e-9) = assertTrue(abs(a - b) < eps, "$a != $b")
    private val N = 4
    private val tau = 1.0
    private val rates = rateLattice(0.05, 0.01, -0.01, N)
    private val probs = constantProbs(0.5, N)

    @Test fun latticeAndMoneyMarket() {
        assertEquals(5, rates.size)
        near(rates[2][2], 0.07)
        near(rates[2][0], 0.03)
        val A = moneyMarket(rates, tau, "ud")
        near(A[1], exp(0.05))
        near(A[2], exp(0.05 + 0.06))
        near(oneStepPrice(0.05, 0.5), exp(-0.025))
    }

    @Test fun bondPricing() {
        val B1 = bondLattice(rates, probs, tau, 1)
        near(B1[0][0], exp(-0.05))
        val B2 = bondLattice(rates, probs, tau, 2)
        near(B2[0][0], exp(-0.05) * (0.5 * exp(-0.06) + 0.5 * exp(-0.04)))
        val B0 = bondPrices(rates, probs, tau, N)
        assertEquals(4, B0.size)
        for (m in 1..N) near(B0[m - 1], bondLattice(rates, probs, tau, m)[0][0])
        for (i in 1 until B0.size) assertTrue(B0[i - 1] > B0[i])
        near(yieldRate(B1[0][0], 0, 1, tau), 0.05)
        val B4 = bondLattice(rates, probs, tau, N)
        for (n in 0 until N) for (k in 0..n)
            near(B4[n][k] * exp(rates[n][k] * tau), 0.5 * B4[n + 1][k + 1] + 0.5 * B4[n + 1][k])
    }

    @Test fun impliedProbabilities() {
        for (p in listOf(0.5, 0.6, 0.35)) {
            val B = bondLattice(rates, constantProbs(p, N), tau, N)
            for (row in impliedProbs(B, rates, tau)) for (q in row) near(q, p)
        }
        near(impliedProb(bondLattice(rates, probs, tau, 2), rates, tau, 0, 0), 0.5)
    }

    @Test fun options() {
        val B4 = bondLattice(rates, probs, tau, N)
        val C = bondOption(rates, probs, tau, N, 2, 0.9, true)
        val P = bondOption(rates, probs, tau, N, 2, 0.9, false)
        near(C[0][0] - P[0][0], B4[0][0] - 0.9 * bondLattice(rates, probs, tau, 2)[0][0])
        val PA = bondOption(rates, probs, tau, N, 2, 0.9, false, american = true)
        assertTrue(PA[0][0] >= P[0][0])
        near(PA[0][0], 0.9 - B4[0][0])
        near(bondOption(rates, probs, tau, N, 2, 0.0, true)[0][0], B4[0][0])
    }

    @Test fun swapCapFloor() {
        val B0 = bondPrices(rates, probs, tau, N)
        val K = swapRate(B0)
        near(payerSwap(rates, probs, tau, N, K)[0][0], 0.0)
        near(payerSwap(rates, probs, tau, N, 0.0)[0][0], 1 - B0.last())
        for (Kx in listOf(0.03, K, 0.07)) {
            val c = cap(rates, probs, tau, N, Kx)[0][0]
            val fo = floor(rates, probs, tau, N, Kx)[0][0]
            near(c - fo, payerSwap(rates, probs, tau, N, Kx)[0][0])
            assertTrue(c >= 0.0 && fo >= 0.0)
        }
        near(plainSum((0 until N).map { caplet(rates, probs, tau, it, 0.05) }), cap(rates, probs, tau, N, 0.05)[0][0])
        near(caplet(rates, probs, tau, 0, 0.05), exp(-0.05) * max(floatingRate(rates, tau, 0, 0) - 0.05, 0.0))
        val V = discountBack(rates, probs, tau, List(N + 1) { 0.0 }, { _, _ -> 1.0 })
        near(V[0][0], plainSum(B0))
    }
}
