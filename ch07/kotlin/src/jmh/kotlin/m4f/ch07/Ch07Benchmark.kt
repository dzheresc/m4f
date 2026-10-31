// Microbenchmarks for chapter 7 (JMH). Run from the chapter directory as ./bench.sh --kotlin.
package m4f.ch07

import org.openjdk.jmh.annotations.Benchmark
import org.openjdk.jmh.annotations.Param
import org.openjdk.jmh.annotations.Scope
import org.openjdk.jmh.annotations.Setup
import org.openjdk.jmh.annotations.State

@State(Scope.Benchmark)
open class Ch07Benchmark {
    @Benchmark
    fun report() = m4f.ch07.report()
}

@State(Scope.Benchmark)
open class Crr {
    @Param("100", "2000")
    var n = 0

    @Benchmark
    fun europeanPut() = crrPrice(100.0, 100.0, 0.05, 0.2, 1.0, n, false)

    @Benchmark
    fun americanPut() = crrPrice(100.0, 100.0, 0.05, 0.2, 1.0, n, false, true)
}
