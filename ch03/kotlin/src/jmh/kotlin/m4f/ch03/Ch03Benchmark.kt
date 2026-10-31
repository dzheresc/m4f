// Microbenchmarks for chapter 3 (JMH). Run from the chapter directory as ./bench.sh --kotlin.
package m4f.ch03

import org.openjdk.jmh.annotations.Benchmark
import org.openjdk.jmh.annotations.Param
import org.openjdk.jmh.annotations.Scope
import org.openjdk.jmh.annotations.Setup
import org.openjdk.jmh.annotations.State

@State(Scope.Benchmark)
open class Ch03Benchmark {
    @Benchmark
    fun report() = m4f.ch03.report()
}

@State(Scope.Benchmark)
open class Binomial {
    @Param("3", "100", "1000")
    var n = 0
    private val tree = BinomialTree(100.0, 0.2, -0.1, 0.05)

    @Benchmark
    fun binomialProb() = binomialProb(n, n / 2, 0.6)

    @Benchmark
    fun expectedPrice() = tree.expectedPrice(n, 0.6)
}
