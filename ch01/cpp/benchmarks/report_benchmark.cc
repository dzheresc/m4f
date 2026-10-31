// End-to-end benchmark: the full chapter report that the ch01 binary prints,
// written to an in-memory stream so that terminal I/O is not measured.
#include <cstdint>
#include <sstream>

#include "benchmark/benchmark.h"
#include "m4f/report.h"

namespace m4f::ch01 {
namespace {

void BM_PrintReport(benchmark::State& state) {
  std::ostringstream os;
  for (auto _ : state) {
    os.str({});
    PrintReport(os);
    benchmark::DoNotOptimize(os);
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<std::int64_t>(os.str().size()));
}
BENCHMARK(BM_PrintReport)->Unit(benchmark::kMicrosecond);

}  // namespace
}  // namespace m4f::ch01
