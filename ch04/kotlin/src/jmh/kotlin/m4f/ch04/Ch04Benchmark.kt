// Microbenchmarks for chapter 4 (JMH). Run from the chapter directory as ./bench.sh --kotlin.
package m4f.ch04

import org.openjdk.jmh.annotations.Benchmark
import org.openjdk.jmh.annotations.Param
import org.openjdk.jmh.annotations.Scope
import org.openjdk.jmh.annotations.Setup
import org.openjdk.jmh.annotations.State
import kotlin.random.Random

/** Holds one more share for every down move so far. */
private fun contrarian(n: Int, moves: List<Int>): Double = 1.0 + moves.count { it == 0 }

@State(Scope.Benchmark)
open class Ch04Benchmark {
    @Benchmark
    fun report() = m4f.ch04.report()
}

@State(Scope.Benchmark)
open class ExpectedDiscountedValue {
    @Param("4", "12", "16")
    var n = 0
    private val q = (0.03 - -0.05) / (0.10 - -0.05)

    @Benchmark
    fun expectedDiscountedValue() =
        expectedDiscountedValue(100.0, 0.10, -0.05, 0.03, n, 100.0, ::contrarian, q)
}

@State(Scope.Benchmark)
open class SelfFinancing {
    @Param("12", "1000")
    var n = 0
    private lateinit var s: List<Double>
    private lateinit var a: List<Double>
    private lateinit var x: List<Double>

    @Setup
    fun setup() {
        val rng = Random(n)
        s = List(n + 1) { 100.0 * (1 + 0.01 * rng.nextDouble(-1.0, 1.0)) }
        a = List(n + 1) { Math.pow(1.001, it.toDouble()) }
        x = List(n) { rng.nextDouble(0.0, 2.0) }
    }

    @Benchmark
    fun selfFinancing() = selfFinancing(100.0, s, a, x)
}
