package m4f.ch10

import kotlin.math.abs
import kotlin.math.exp
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class Ch10Test {
    private fun near(a: Double, b: Double, eps: Double = 1e-7) = assertTrue(abs(a - b) < eps, "$a != $b")
    private val Ts = listOf(0.5, 1.0, 1.5, 2.0, 2.5, 3.0)
    private val Bs = listOf(0.978, 0.953, 0.926, 0.898, 0.870, 0.842)

    @Test fun priceAndDuration() {
        val z = zeroCoupon(100.0, 4.0)
        near(price(z, 0.06), 100 * exp(-0.24))
        near(duration(z, 0.06), 4.0)
        near(duration(z, 0.06, 1.5), 2.5)
        val b = couponBond(100.0, 5.0, 4)
        assertEquals(listOf(1.0, 2.0, 3.0, 4.0), b.map { it.t })
        assertEquals(listOf(5.0, 5.0, 5.0, 105.0), b.map { it.C })
        val h = 1e-5
        near((price(b, 0.06 + h) - price(b, 0.06 - h)) / (2 * h), -duration(b, 0.06) * price(b, 0.06), 1e-4)
        assertTrue(duration(b, 0.06) < 4.0)
    }

    @Test fun investmentValue() {
        val b = couponBond(100.0, 5.0, 4)
        for (t in listOf(1.0, 2.5, 4.0)) {
            near(investmentValue(b, 0.06, t), price(b, 0.06) * exp(0.06 * t))
            near(logReturn(b, 0.06, 0.06, t), 0.06)
        }
        val D = duration(b, 0.06)
        assertTrue(abs(investmentValue(b, 0.061, D) - investmentValue(b, 0.06, D)) < 1e-2)
        assertTrue(investmentValue(b, 0.07, 1.0) < investmentValue(b, 0.06, 1.0))
        assertTrue(investmentValue(b, 0.07, 4.0) > investmentValue(b, 0.06, 4.0))
    }

    @Test fun immunisation() {
        val A = couponBond(100.0, 4.0, 5)
        val B = couponBond(100.0, 8.0, 10)
        val (w1, w2) = immunisingWeights(duration(A, 0.06), duration(B, 0.06), 5.0)
        near(w1 + w2, 1.0)
        near(w1 * duration(A, 0.06) + w2 * duration(B, 0.06), 5.0)
        val p = immunise(1000.0, A, B, 0.06, 0.0, 5.0)
        near(value(p, A, B, 0.06, 0.0), 1000.0)
        val target = 1000.0 * exp(0.06 * 5)
        for (y in listOf(0.05, 0.07))
            assertTrue(p.q1 * investmentValue(A, y, 5.0) + p.q2 * investmentValue(B, y, 5.0) > target - 1e-9)
    }

    @Test fun dynamicHedgeFlatPath() {
        val A = couponBond(100.0, 4.0, 5)
        val B = couponBond(100.0, 8.0, 10)
        val flat = (0..10).map { 0.5 * it to 0.06 }
        val steps = dynamicHedge(1000.0, A, B, 5.0, flat)
        assertEquals(11, steps.size)
        near(steps.last().value, 1000.0 * exp(0.3), 1e-6)
        for (st in steps.dropLast(1)) near(st.duration, 5.0 - st.t, 1e-9)
        near(dynamicHedge(1000.0, A, B, 5.0, flat, rebalance = false).last().value, 1000.0 * exp(0.3), 1e-6)
    }

    @Test fun termStructure() {
        val fw = forwardRates(Ts, Bs)
        near(fw[0], yieldRate(Bs[0], 0.5))
        val back = pricesFromForwardRates(Ts, fw)
        for (i in Ts.indices) near(back[i], Bs[i])
        near(forwardRate(Bs[1], 1.0, Bs[5], 3.0), (fw[2] + fw[3] + fw[4] + fw[5]) / 4)
        near(impliedPrice(Bs[1], Bs[3]), exp(-forwardRate(Bs[1], 1.0, Bs[3], 2.0)))
        val As = moneyMarketAccount(Ts, Bs)
        for (i in Ts.indices) near(As[i], 1 / Bs[i])
        val P = couponBondPrice(listOf(1.0, 2.0, 3.0), listOf(6.0, 6.0, 106.0), Ts, Bs)
        near(P, 6 * Bs[1] + 6 * Bs[3] + 106 * Bs[5])
        val y = yieldToMaturity(P, listOf(1.0, 2.0, 3.0), listOf(6.0, 6.0, 106.0))
        near(6 * exp(-y) + 6 * exp(-2 * y) + 106 * exp(-3 * y), P, 1e-8)
        near(yieldToMaturity(Bs[5] * 100, listOf(3.0), listOf(100.0)), yieldRate(Bs[5], 3.0), 1e-9)
    }
}
