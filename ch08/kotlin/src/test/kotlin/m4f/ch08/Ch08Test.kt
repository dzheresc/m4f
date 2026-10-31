package m4f.ch08

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.max
import kotlin.math.pow
import kotlin.test.Test
import kotlin.test.assertTrue

class Ch08Test {
    private fun assertNear(expected: Double, actual: Double, eps: Double = 1e-9) =
        assertTrue(abs(expected - actual) < eps, "expected $expected, got $actual")

    private fun call(strike: Double): Payoff = { s -> max(s - strike, 0.0) }

    private fun put(strike: Double): Payoff = { s -> max(strike - s, 0.0) }

    @Test
    fun oneStep() {
        val rep = oneStepReplication(100.0, 0.2, -0.1, 0.1, 20.0, 0.0)
        assertNear(20.0, rep.x * 120 + rep.y * 1.1)
        assertNear(0.0, rep.x * 90 + rep.y * 1.1)
        assertNear(40 / 3.3, rep.price)
    }

    @Test
    fun replication() {
        val values = optionTree(100.0, 0.2, -0.1, 0.1, 3, call(100.0))
        for (n in 0 until 3) {
            for (k in 0..n) {
                val (x, y) = replicatingPosition(100.0, 0.2, -0.1, 0.1, values, n, k)
                // Replicates both successors.
                for (nextK in listOf(k, k + 1)) {
                    assertNear(
                        values[n + 1][nextK],
                        x * stockPrice(100.0, 0.2, -0.1, n + 1, nextK) + y * 1.1.pow(n + 1),
                    )
                }
            }
        }
    }

    @Test
    fun sumAndCrr() {
        for (numSteps in listOf(1, 2, 5, 8)) {
            for (strike in listOf(70.0, 100.0, 130.0)) {
                val tree = optionTree(100.0, 0.2, -0.1, 0.1, numSteps, call(strike))[0][0]
                assertNear(tree, europeanPriceSum(100.0, 0.2, -0.1, 0.1, numSteps, call(strike)))
                assertNear(tree, crrCall(100.0, strike, 0.2, -0.1, 0.1, numSteps).price)
                val put = optionTree(100.0, 0.2, -0.1, 0.1, numSteps, put(strike))[0][0]
                // Put-call parity.
                assertNear(100 - strike * 1.1.pow(-numSteps), tree - put)
            }
        }
        assertNear(1.0, binomialSum(0.3, 6, 0))
    }

    @Test
    fun american() {
        val american = optionTree(100.0, 0.1, -0.1, 0.02, 4, put(100.0), true)
        val european = optionTree(100.0, 0.1, -0.1, 0.02, 4, put(100.0))
        assertTrue(american[0][0] > european[0][0])
        for (n in 0 until 5) {
            for (k in 0..n) {
                val payoff = max(100 - stockPrice(100.0, 0.1, -0.1, n, k), 0.0)
                assertTrue(american[n][k] >= payoff - 1e-12)
            }
        }
        assertTrue(earlyExerciseNodes(100.0, 0.1, -0.1, 0.02, 4, call(100.0)).isEmpty())
        assertNear(
            optionTree(100.0, 0.1, -0.1, 0.02, 4, call(100.0))[0][0],
            optionTree(100.0, 0.1, -0.1, 0.02, 4, call(100.0), true)[0][0],
        )
    }

    @Test
    fun blackScholes() {
        assertNear(0.5, normCdf(0.0), 1e-14)
        assertNear(0.975, normCdf(1.959963984540054), 1e-12)
        assertNear(1.0, normCdf(-8.0) + normCdf(8.0), 1e-14)
        val call = bsCall(100.0, 100.0, 0.05, 0.2, 1.0)
        val put = bsPut(100.0, 100.0, 0.05, 0.2, 1.0)
        assertNear(10.450583572185565, call, 1e-10)
        assertNear(100 - 100 * exp(-0.05), call - put)
        val h = 1e-4
        val finiteDiff =
            (bsCall(100 + h, 100.0, 0.05, 0.2, 1.0) - bsCall(100 - h, 100.0, 0.05, 0.2, 1.0)) /
                (2 * h)
        assertNear(finiteDiff, bsCallDelta(100.0, 100.0, 0.05, 0.2, 1.0), 1e-6)
        assertNear(call, crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, 2000, true), 1e-2)
    }
}
