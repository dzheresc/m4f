// Microbenchmarks for chapter 11 (JMH). Run from the chapter directory as ./bench.sh --kotlin.
package m4f.ch11

import org.openjdk.jmh.annotations.Benchmark
import org.openjdk.jmh.annotations.Param
import org.openjdk.jmh.annotations.Scope
import org.openjdk.jmh.annotations.Setup
import org.openjdk.jmh.annotations.State

private const val TAU = 1.0 / 12

@State(Scope.Benchmark)
open class Ch11Benchmark {
    @Benchmark
    fun report() = m4f.ch11.report()
}

@State(Scope.Benchmark)
open class Curve {
    @Param("10", "50", "200")
    var n = 0
    private lateinit var rates: Lattice
    private lateinit var probs: Lattice

    @Setup
    fun setup() {
        rates = rateLattice(0.05, 0.002, -0.002, n)
        probs = constantProbs(0.5, n)
    }

    @Benchmark
    fun bondPrices() = bondPrices(rates, probs, TAU, n)
}

@State(Scope.Benchmark)
open class Lattices {
    @Param("100", "1000")
    var n = 0
    private lateinit var rates: Lattice
    private lateinit var probs: Lattice

    @Setup
    fun setup() {
        rates = rateLattice(0.05, 0.002, -0.002, n)
        probs = constantProbs(0.5, n)
    }

    @Benchmark
    fun caplet() = caplet(rates, probs, TAU, n - 1, 0.004)

    @Benchmark
    fun rateLattice() = rateLattice(0.05, 0.002, -0.002, n)
}
