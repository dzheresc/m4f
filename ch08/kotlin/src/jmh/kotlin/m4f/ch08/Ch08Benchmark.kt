// Microbenchmarks for chapter 8 (JMH). Run from the chapter directory as ./bench.sh --kotlin.
package m4f.ch08

import org.openjdk.jmh.annotations.Benchmark
import org.openjdk.jmh.annotations.Param
import org.openjdk.jmh.annotations.Scope
import org.openjdk.jmh.annotations.Setup
import org.openjdk.jmh.annotations.State
import kotlin.math.exp
import kotlin.math.max
import kotlin.math.sqrt

private fun put(s: Double) = max(100.0 - s, 0.0)

@State(Scope.Benchmark)
open class Ch08Benchmark {
    @Benchmark
    fun binomialSum() = binomialSum(0.5, 1000, 500)

    @Benchmark
    fun report() = m4f.ch08.report()
}

@State(Scope.Benchmark)
open class Trees {
    @Param("100", "2000")
    var n = 0
    private var u = 0.0
    private var d = 0.0
    private var r = 0.0

    /** A one-year CRR tree, sigma = 0.2, r = 0.05. */
    @Setup
    fun setup() {
        val tau = 1.0 / n
        u = exp(0.2 * sqrt(tau)) - 1
        d = exp(-0.2 * sqrt(tau)) - 1
        r = exp(0.05 * tau) - 1
    }

    @Benchmark
    fun optionTreeAmerican() = optionTree(100.0, u, d, r, n, ::put, american = true)

    @Benchmark
    fun earlyExerciseNodes() = earlyExerciseNodes(100.0, u, d, r, n, ::put)

    @Benchmark
    fun crrTreePriceEuropean() = crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, n, false)

    @Benchmark
    fun crrTreePriceAmerican() = crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, n, false, true)
}
