package m4f.ch10

import kotlin.math.abs
import kotlin.math.exp
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class Ch10Test {
    private val maturities = listOf(0.5, 1.0, 1.5, 2.0, 2.5, 3.0)
    private val prices = listOf(0.978, 0.953, 0.926, 0.898, 0.870, 0.842)

    private fun assertNear(expected: Double, actual: Double, eps: Double = 1e-7) =
        assertTrue(abs(expected - actual) < eps, "expected $expected, got $actual")

    @Test
    fun priceAndDuration() {
        val zero = zeroCoupon(100.0, 4.0)
        assertNear(100 * exp(-0.24), price(zero, 0.06))
        assertNear(4.0, duration(zero, 0.06))
        assertNear(2.5, duration(zero, 0.06, 1.5))
        val b = couponBond(100.0, 5.0, 4)
        assertEquals(listOf(1.0, 2.0, 3.0, 4.0), b.map { it.t })
        assertEquals(listOf(5.0, 5.0, 5.0, 105.0), b.map { it.amount })
        val h = 1e-5
        val slope = (price(b, 0.06 + h) - price(b, 0.06 - h)) / (2 * h)
        assertNear(-duration(b, 0.06) * price(b, 0.06), slope, 1e-4)
        assertTrue(duration(b, 0.06) < 4.0)
    }

    @Test
    fun investmentValue() {
        val b = couponBond(100.0, 5.0, 4)
        for (t in listOf(1.0, 2.5, 4.0)) {
            assertNear(price(b, 0.06) * exp(0.06 * t), investmentValue(b, 0.06, t))
            assertNear(0.06, logReturn(b, 0.06, 0.06, t))
        }
        val d = duration(b, 0.06)
        assertTrue(abs(investmentValue(b, 0.061, d) - investmentValue(b, 0.06, d)) < 1e-2)
        assertTrue(investmentValue(b, 0.07, 1.0) < investmentValue(b, 0.06, 1.0))
        assertTrue(investmentValue(b, 0.07, 4.0) > investmentValue(b, 0.06, 4.0))
    }

    @Test
    fun immunisation() {
        val bondA = couponBond(100.0, 4.0, 5)
        val bondB = couponBond(100.0, 8.0, 10)
        val (w1, w2) = immunisingWeights(duration(bondA, 0.06), duration(bondB, 0.06), 5.0)
        assertNear(1.0, w1 + w2)
        assertNear(5.0, w1 * duration(bondA, 0.06) + w2 * duration(bondB, 0.06))
        val p = immunise(1000.0, bondA, bondB, 0.06, 0.0, 5.0)
        assertNear(1000.0, portfolioValue(p, bondA, bondB, 0.06, 0.0))
        val target = 1000.0 * exp(0.06 * 5)
        for (y in listOf(0.05, 0.07)) {
            val v = p.q1 * investmentValue(bondA, y, 5.0) + p.q2 * investmentValue(bondB, y, 5.0)
            assertTrue(v > target - 1e-9)
        }
    }

    @Test
    fun dynamicHedgeFlatPath() {
        val bondA = couponBond(100.0, 4.0, 5)
        val bondB = couponBond(100.0, 8.0, 10)
        val flat = (0..10).map { 0.5 * it to 0.06 }
        val steps = dynamicHedge(1000.0, bondA, bondB, 5.0, flat)
        assertEquals(11, steps.size)
        assertNear(1000.0 * exp(0.3), steps.last().value, 1e-6)
        for (st in steps.dropLast(1)) assertNear(5.0 - st.t, st.duration, 1e-9)
        val fixed = dynamicHedge(1000.0, bondA, bondB, 5.0, flat, rebalance = false)
        assertNear(1000.0 * exp(0.3), fixed.last().value, 1e-6)
    }

    @Test
    fun termStructure() {
        val forwards = forwardRates(maturities, prices)
        assertNear(yieldRate(prices[0], 0.5), forwards[0])
        val rebuilt = pricesFromForwardRates(maturities, forwards)
        for (i in maturities.indices) assertNear(prices[i], rebuilt[i])
        assertNear(
            (forwards[2] + forwards[3] + forwards[4] + forwards[5]) / 4,
            forwardRate(prices[1], 1.0, prices[5], 3.0),
        )
        assertNear(
            exp(-forwardRate(prices[1], 1.0, prices[3], 2.0)),
            impliedPrice(prices[1], prices[3]),
        )
        val account = moneyMarketAccount(maturities, prices)
        for (i in maturities.indices) assertNear(1 / prices[i], account[i])
        val flowTimes = listOf(1.0, 2.0, 3.0)
        val flowAmounts = listOf(6.0, 6.0, 106.0)
        val p = couponBondPrice(flowTimes, flowAmounts, maturities, prices)
        assertNear(6 * prices[1] + 6 * prices[3] + 106 * prices[5], p)
        val y = yieldToMaturity(p, flowTimes, flowAmounts)
        assertNear(p, 6 * exp(-y) + 6 * exp(-2 * y) + 106 * exp(-3 * y), 1e-8)
        assertNear(
            yieldRate(prices[5], 3.0),
            yieldToMaturity(prices[5] * 100, listOf(3.0), listOf(100.0)),
            1e-9,
        )
    }
}
