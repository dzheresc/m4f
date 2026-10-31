package m4f.ch02

import kotlin.math.abs
import kotlin.math.pow
import kotlin.test.Test
import kotlin.test.assertTrue

class Ch02Test {
    private fun assertNear(expected: Double, actual: Double, eps: Double = 1e-9) =
        assertTrue(abs(expected - actual) < eps, "expected $expected, got $actual")

    @Test
    fun reportValues() {
        assertNear(6144.567105, annuityPresentValue(1000.0, 0.10, 10), 1e-6)
        assertNear(644.301401, loanInstallment(100000.0, 0.005, 300), 1e-6)
        assertNear(0.119404, continuousFromPeriodic(0.12, 12), 1e-6)
        assertNear(4.0, moneyMarketRollover(1.0, listOf(0.5, 0.5)))
    }

    @Test
    fun interestIdentities() {
        assertNear(100.0, simplePresentValue(simpleValue(100.0, 0.1, 0.5), 0.1, 0.5))
        assertNear(0.08, simpleRate(100.0, simpleValue(100.0, 0.08, 2.0), 2.0))
        assertNear(0.1, effectiveRate(0.1, 1))
        assertNear(0.1, periodicFromContinuous(continuousFromPeriodic(0.1, 12), 12))
        assertNear(0.1, periodicFromPeriodic(periodicFromPeriodic(0.1, 4, 12), 12, 4))
        assertNear(0.1, continuousEffectiveRate(continuousFromPeriodic(0.1, 1)))
        assertNear(continuousValue(100.0, 0.1, 1.0), periodicValue(100.0, 0.1, 1000000, 1.0), 1e-4)
        assertNear(1000.0, annuityPresentValue(loanInstallment(1000.0, 0.1, 5), 0.1, 5))
        assertNear(annuityFactor(0.1, 5) * 1.1.pow(5), annuityFutureValue(1.0, 0.1, 5))
    }

    @Test
    fun bondIdentities() {
        for (m in listOf(1, 2, 12, null)) {
            assertNear(0.08, impliedRate(zeroCouponPrice(0.08, 2.5, m), 2.5, m))
        }
        val price = couponBondPrice(100.0, 10.0, 3, 0.12)
        assertNear(0.12, yieldToMaturity(price, 100.0, 10.0, 3), 1e-8)
        assertNear(100.0, couponBondPrice(100.0, 10.0, 3, 0.10))
    }

    /** Closed-form price and Newton solver against a direct sum, including y = 0 and tiny y. */
    @Test
    fun closedFormAndNewton() {
        for (n in listOf(1, 3, 60, 360)) {
            for (y in listOf(-0.5, -1e-9, 0.0, 1e-9, 0.004, 0.12, 2.0)) {
                var direct = 100 * (1 + y).pow(-n)
                for (k in 1..n) direct += 5 * (1 + y).pow(-k)
                assertNear(direct, couponBondPrice(100.0, 5.0, n, y), 1e-9 * direct)
                assertNear(y, yieldToMaturity(direct, 100.0, 5.0, n), 1e-9)
            }
        }
        assertNear(-0.99, yieldToMaturity(1e9, 100.0, 5.0, 3), 1e-9)
    }
}
