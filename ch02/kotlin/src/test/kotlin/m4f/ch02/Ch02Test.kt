package m4f.ch02

import kotlin.math.abs
import kotlin.math.pow
import kotlin.test.Test
import kotlin.test.assertTrue

class Ch02Test {
    private fun near(a: Double, b: Double, eps: Double = 1e-9) = assertTrue(abs(a - b) < eps, "$a != $b")

    @Test fun reportValues() {
        near(annuityPresentValue(1000.0, 0.10, 10), 6144.567105, 1e-6)
        near(loanInstallment(100000.0, 0.005, 300), 644.301401, 1e-6)
        near(continuousFromPeriodic(0.12, 12), 0.119404, 1e-6)
        near(moneyMarketRollover(1.0, listOf(0.5, 0.5)), 4.0)
    }

    @Test fun libraryIdentities() {
        near(simplePresentValue(simpleValue(100.0, 0.1, 0.5), 0.1, 0.5), 100.0)
        near(simpleRate(100.0, simpleValue(100.0, 0.08, 2.0), 2.0), 0.08)
        near(effectiveRate(0.1, 1), 0.1)
        near(periodicFromContinuous(continuousFromPeriodic(0.1, 12), 12), 0.1)
        near(periodicFromPeriodic(periodicFromPeriodic(0.1, 4, 12), 12, 4), 0.1)
        near(continuousEffectiveRate(continuousFromPeriodic(0.1, 1)), 0.1)
        near(periodicValue(100.0, 0.1, 1000000, 1.0), continuousValue(100.0, 0.1, 1.0), 1e-4)
        near(annuityPresentValue(loanInstallment(1000.0, 0.1, 5), 0.1, 5), 1000.0)
        near(annuityFutureValue(1.0, 0.1, 5), annuityFactor(0.1, 5) * 1.1.pow(5))
        for (m in listOf(1, 2, 12, null)) near(impliedRate(zeroCouponPrice(0.08, 2.5, m), 2.5, m), 0.08)
        near(yieldToMaturity(couponBondPrice(100.0, 10.0, 3, 0.12), 100.0, 10.0, 3), 0.12, 1e-8)
        near(couponBondPrice(100.0, 10.0, 3, 0.10), 100.0)
    }
}
