// Microbenchmarks for chapter 10 (JMH). Run from the chapter directory as ./bench.sh --kotlin.
package m4f.ch10

import org.openjdk.jmh.annotations.Benchmark
import org.openjdk.jmh.annotations.Param
import org.openjdk.jmh.annotations.Scope
import org.openjdk.jmh.annotations.Setup
import org.openjdk.jmh.annotations.State
import kotlin.math.exp
import kotlin.math.sqrt

@State(Scope.Benchmark)
open class Ch10Benchmark {
    @Benchmark
    fun report() = m4f.ch10.report()
}

/** A bond paying a coupon at every maturity of a smooth zero curve of [n] annual maturities. */
@State(Scope.Benchmark)
open class Curve {
    @Param("3", "30", "360")
    var n = 0
    private lateinit var maturities: List<Double>
    private lateinit var prices: List<Double>
    private lateinit var amounts: List<Double>
    private var price = 0.0

    @Setup
    fun setup() {
        maturities = List(n) { (it + 1).toDouble() }
        prices = maturities.map { t -> exp(-(0.03 + 0.02 * (1 - exp(-t / 5))) * t) }
        amounts = List(n) { if (it == n - 1) 105.0 else 5.0 }
        price = couponBondPrice(maturities, amounts, maturities, prices)
    }

    @Benchmark
    fun couponBondPrice() = couponBondPrice(maturities, amounts, maturities, prices)

    @Benchmark
    fun yieldToMaturity() = yieldToMaturity(price, maturities, amounts)
}

@State(Scope.Benchmark)
open class Hedge {
    @Param("5", "260")
    var steps = 0
    private val b1 = couponBond(100.0, 8.0, 2)
    private val b2 = couponBond(100.0, 6.0, 10)
    private lateinit var path: List<Pair<Double, Double>>

    @Setup
    fun setup() {
        val rng = java.util.Random(steps.toLong())
        val sd = 0.01 / sqrt(steps / 5.0)
        val p = arrayListOf(0.0 to 0.06)
        for (i in 1..steps) {
            p.add(5.0 * i / steps to p.last().second + sd * rng.nextGaussian())
        }
        path = p
    }

    @Benchmark
    fun dynamicHedge() = dynamicHedge(1000.0, b1, b2, 5.0, path)
}
