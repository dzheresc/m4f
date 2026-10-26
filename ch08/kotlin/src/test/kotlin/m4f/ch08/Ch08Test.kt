package m4f.ch08

import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.max
import kotlin.math.pow
import kotlin.test.Test
import kotlin.test.assertTrue

class Ch08Test {
    private fun near(a: Double, b: Double, eps: Double = 1e-9) = assertTrue(abs(a - b) < eps, "$a != $b")
    private fun call(X: Double): Payoff = { s -> max(s - X, 0.0) }
    private fun put(X: Double): Payoff = { s -> max(X - s, 0.0) }

    @Test fun oneStep() {
        val rep = oneStepReplication(100.0, 0.2, -0.1, 0.1, 20.0, 0.0)
        near(rep.x * 120 + rep.y * 1.1, 20.0)
        near(rep.x * 90 + rep.y * 1.1, 0.0)
        near(rep.price, 40 / 3.3)
    }

    @Test fun replication() {
        val V = optionTree(100.0, 0.2, -0.1, 0.1, 3, call(100.0))
        for (n in 0 until 3)
            for (k in 0..n) {
                val (x, y) = replicatingPosition(100.0, 0.2, -0.1, 0.1, V, n, k)
                for (kk in listOf(k, k + 1))
                    near(x * stockPrice(100.0, 0.2, -0.1, n + 1, kk) + y * 1.1.pow(n + 1), V[n + 1][kk])
            }
    }

    @Test fun sumAndCrr() {
        for (N in listOf(1, 2, 5, 8))
            for (X in listOf(70.0, 100.0, 130.0)) {
                val tree = optionTree(100.0, 0.2, -0.1, 0.1, N, call(X))[0][0]
                near(europeanPriceSum(100.0, 0.2, -0.1, 0.1, N, call(X)), tree)
                near(crrCall(100.0, X, 0.2, -0.1, 0.1, N).price, tree)
                val P = optionTree(100.0, 0.2, -0.1, 0.1, N, put(X))[0][0]
                near(tree - P, 100 - X * 1.1.pow(-N))
            }
        near(binomialSum(0.3, 6, 0), 1.0)
    }

    @Test fun american() {
        val VA = optionTree(100.0, 0.1, -0.1, 0.02, 4, put(100.0), true)
        val VE = optionTree(100.0, 0.1, -0.1, 0.02, 4, put(100.0))
        assertTrue(VA[0][0] > VE[0][0])
        for (n in 0 until 5)
            for (k in 0..n) assertTrue(VA[n][k] >= max(100 - stockPrice(100.0, 0.1, -0.1, n, k), 0.0) - 1e-12)
        assertTrue(earlyExerciseNodes(100.0, 0.1, -0.1, 0.02, 4, call(100.0)).isEmpty())
        near(optionTree(100.0, 0.1, -0.1, 0.02, 4, call(100.0), true)[0][0],
            optionTree(100.0, 0.1, -0.1, 0.02, 4, call(100.0))[0][0])
    }

    @Test fun blackScholes() {
        near(normCdf(0.0), 0.5, 1e-14)
        near(normCdf(1.959963984540054), 0.975, 1e-12)
        near(normCdf(-8.0) + normCdf(8.0), 1.0, 1e-14)
        val C = bsCall(100.0, 100.0, 0.05, 0.2, 1.0)
        val P = bsPut(100.0, 100.0, 0.05, 0.2, 1.0)
        near(C, 10.450583572185565, 1e-10)
        near(C - P, 100 - 100 * exp(-0.05))
        val h = 1e-4
        val fd = (bsCall(100 + h, 100.0, 0.05, 0.2, 1.0) - bsCall(100 - h, 100.0, 0.05, 0.2, 1.0)) / (2 * h)
        near(bsCallDelta(100.0, 100.0, 0.05, 0.2, 1.0), fd, 1e-6)
        near(crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, 2000, true), C, 1e-2)
    }
}
