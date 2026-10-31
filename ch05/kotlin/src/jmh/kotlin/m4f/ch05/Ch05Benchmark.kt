// Microbenchmarks for chapter 5 (JMH). Run from the chapter directory as ./bench.sh --kotlin.
package m4f.ch05

import org.openjdk.jmh.annotations.Benchmark
import org.openjdk.jmh.annotations.Param
import org.openjdk.jmh.annotations.Scope
import org.openjdk.jmh.annotations.Setup
import org.openjdk.jmh.annotations.State
import kotlin.random.Random

/** A positive definite covariance matrix of [n] assets from a two-factor model. */
private fun makeCov(n: Int): Mat {
    val rng = Random(n)
    val factors = List(n) { listOf(rng.nextDouble(-0.2, 0.3), rng.nextDouble(-0.2, 0.3)) }
    return List(n) { i ->
        val idio = rng.nextDouble(0.01, 0.04)
        List(n) { j ->
            factors[i][0] * factors[j][0] + factors[i][1] * factors[j][1] +
                if (i == j) idio else 0.0
        }
    }
}

@State(Scope.Benchmark)
open class Ch05Benchmark {
    @Benchmark
    fun report() = m4f.ch05.report()
}

@State(Scope.Benchmark)
open class Assets {
    @Param("3", "50", "200")
    var n = 0
    private lateinit var cov: Mat
    private lateinit var w: Vec

    @Setup
    fun setup() {
        cov = makeCov(n)
        w = List(n) { 1.0 / n }
    }

    @Benchmark
    fun minVariancePortfolio() = minVariancePortfolio(cov)

    @Benchmark
    fun vecMat() = vecMat(w, cov)
}

@State(Scope.Benchmark)
open class Scenarios {
    @Param("3", "10000")
    var n = 0
    private lateinit var k1: List<Double>
    private lateinit var k2: List<Double>
    private lateinit var p: List<Double>

    @Setup
    fun setup() {
        val rng = Random(n)
        k1 = List(n) { rng.nextDouble(-0.1, 0.2) }
        k2 = List(n) { rng.nextDouble(-0.1, 0.2) }
        p = List(n) { 1.0 / n }
    }

    @Benchmark
    fun correlation() = correlation(k1, k2, p)
}
