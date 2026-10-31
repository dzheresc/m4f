package m4f.ch07

import kotlin.math.abs
import kotlin.math.exp
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class Ch07Test {
    private fun assertNear(expected: Double, actual: Double, eps: Double = 1e-9) =
        assertTrue(abs(expected - actual) < eps, "expected $expected, got $actual")

    private fun price(
        s0: Double,
        strike: Double,
        maturity: Double,
        isCall: Boolean,
        american: Boolean = false,
    ) = crrPrice(s0, strike, 0.05, 0.2, maturity, 200, isCall, american)

    @Test
    fun payoffsAndParity() {
        for (s in listOf(80.0, 100.0, 120.0)) {
            assertNear(s - 100, callPayoff(s, 100.0) - putPayoff(s, 100.0))
        }
        assertNear(-10 * exp(0.05), profit(0.0, 10.0, 0.05, 1.0))
        val call = price(100.0, 100.0, 1.0, true)
        val put = price(100.0, 100.0, 1.0, false)
        assertNear(0.0, parityGap(call, put, 100.0, 100.0, 0.05, 1.0))
        assertNear(7 - 100 + 100 * exp(-0.05), parityGap(12.0, 5.0, 100.0, 100.0, 0.05, 1.0))
    }

    @Test
    fun bounds() {
        for (s0 in listOf(80.0, 100.0, 120.0)) {
            for (maturity in listOf(0.25, 1.0)) {
                val callBounds = europeanCallBounds(s0, 100.0, 0.05, maturity)
                val putBounds = europeanPutBounds(s0, 100.0, 0.05, maturity)
                assertEquals(0.0, boundViolation(price(s0, 100.0, maturity, true), callBounds))
                assertEquals(0.0, boundViolation(price(s0, 100.0, maturity, false), putBounds))
                val b = americanParityBounds(s0, 100.0, 0.05, maturity)
                val d = price(s0, 100.0, maturity, true, true) -
                    price(s0, 100.0, maturity, false, true)
                assertTrue(d in b.lo..b.hi)
            }
        }
        assertNear(0.5, boundViolation(3.5, Bounds(4.0, 100.0)))
        assertNear(1.0, boundViolation(101.0, Bounds(4.0, 100.0)))
    }

    @Test
    fun american() {
        for (s0 in listOf(80.0, 100.0, 120.0)) {
            assertNear(price(s0, 100.0, 1.0, true), price(s0, 100.0, 1.0, true, true))
            assertTrue(price(s0, 100.0, 1.0, false, true) >= price(s0, 100.0, 1.0, false))
            assertTrue(price(s0, 100.0, 1.0, false, true) >= putPayoff(s0, 100.0))
        }
        val americanPuts = listOf(0.25, 0.5, 1.0, 2.0).map { price(100.0, 100.0, it, false, true) }
        assertEquals(americanPuts.sorted(), americanPuts)
    }

    @Test
    fun strikeDependence() {
        val calls = listOf(90.0, 100.0, 110.0).map { price(100.0, it, 1.0, true) }
        assertTrue(calls[0] > calls[1] && calls[1] > calls[2])
        assertTrue(butterflyGap(calls[0], calls[1], calls[2], 90.0, 100.0, 110.0) <= 0)
        assertTrue(calls[0] - calls[1] <= 10 * exp(-0.05))
        assertNear(1.5, butterflyGap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0))
    }
}
