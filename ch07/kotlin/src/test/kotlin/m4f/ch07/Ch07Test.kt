package m4f.ch07

import kotlin.math.abs
import kotlin.math.exp
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class Ch07Test {
    private fun near(a: Double, b: Double, eps: Double = 1e-9) = assertTrue(abs(a - b) < eps, "$a != $b")
    private fun price(S0: Double, X: Double, T: Double, isCall: Boolean, american: Boolean = false) =
        crrPrice(S0, X, 0.05, 0.2, T, 200, isCall, american)

    @Test fun payoffsAndParity() {
        for (S in listOf(80.0, 100.0, 120.0)) near(callPayoff(S, 100.0) - putPayoff(S, 100.0), S - 100)
        near(profit(0.0, 10.0, 0.05, 1.0), -10 * exp(0.05))
        near(parityGap(price(100.0, 100.0, 1.0, true), price(100.0, 100.0, 1.0, false), 100.0, 100.0, 0.05, 1.0), 0.0)
        near(parityGap(12.0, 5.0, 100.0, 100.0, 0.05, 1.0), 7 - 100 + 100 * exp(-0.05))
    }

    @Test fun bounds() {
        for (S0 in listOf(80.0, 100.0, 120.0))
            for (T in listOf(0.25, 1.0)) {
                assertEquals(0.0, boundViolation(price(S0, 100.0, T, true), europeanCallBounds(S0, 100.0, 0.05, T)))
                assertEquals(0.0, boundViolation(price(S0, 100.0, T, false), europeanPutBounds(S0, 100.0, 0.05, T)))
                val b = americanParityBounds(S0, 100.0, 0.05, T)
                val d = price(S0, 100.0, T, true, true) - price(S0, 100.0, T, false, true)
                assertTrue(d in b.lo..b.hi)
            }
        near(boundViolation(3.5, Bounds(4.0, 100.0)), 0.5)
        near(boundViolation(101.0, Bounds(4.0, 100.0)), 1.0)
    }

    @Test fun american() {
        for (S0 in listOf(80.0, 100.0, 120.0)) {
            near(price(S0, 100.0, 1.0, true, true), price(S0, 100.0, 1.0, true))
            assertTrue(price(S0, 100.0, 1.0, false, true) >= price(S0, 100.0, 1.0, false))
            assertTrue(price(S0, 100.0, 1.0, false, true) >= putPayoff(S0, 100.0))
        }
        val Ps = listOf(0.25, 0.5, 1.0, 2.0).map { price(100.0, 100.0, it, false, true) }
        assertEquals(Ps.sorted(), Ps)
    }

    @Test fun strikeDependence() {
        val Cs = listOf(90.0, 100.0, 110.0).map { price(100.0, it, 1.0, true) }
        assertTrue(Cs[0] > Cs[1] && Cs[1] > Cs[2])
        assertTrue(butterflyGap(Cs[0], Cs[1], Cs[2], 90.0, 100.0, 110.0) <= 0)
        assertTrue(Cs[0] - Cs[1] <= 10 * exp(-0.05))
        near(butterflyGap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0), 1.5)
    }
}
