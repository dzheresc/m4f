// Microbenchmarks for chapter 2 (JMH). Run from the chapter directory as ./bench.sh --kotlin.
package m4f.ch02

import org.openjdk.jmh.annotations.Benchmark
import org.openjdk.jmh.annotations.Param
import org.openjdk.jmh.annotations.Scope
import org.openjdk.jmh.annotations.Setup
import org.openjdk.jmh.annotations.State

@State(Scope.Benchmark)
open class Ch02Benchmark {
    @Benchmark
    fun report() = m4f.ch02.report()
}

@State(Scope.Benchmark)
open class CouponBond {
    @Param("3", "30", "360")
    var n = 0
    private var price = 0.0

    @Setup
    fun setup() {
        price = couponBondPrice(100.0, 8.0, n, 0.07)
    }

    @Benchmark
    fun couponBondPrice() = couponBondPrice(100.0, 8.0, n, 0.07)

    @Benchmark
    fun yieldToMaturity() = yieldToMaturity(price, 100.0, 8.0, n)
}
