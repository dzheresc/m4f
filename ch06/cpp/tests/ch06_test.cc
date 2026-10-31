// Unit tests for chapter 6. A tiny self-contained harness keeps the build free
// of third-party dependencies; each check mirrors a Kotlin and Python test.
#include <cmath>
#include <cstdio>

#include "m4f/forwards.h"
#include "m4f/futures.h"

namespace m4f::ch06 {
namespace {

class Checker {
 public:
  void Expect(bool ok, const char* what) {
    if (!ok) {
      std::printf("FAIL: %s\n", what);
      ++failures_;
    }
  }
  int failures() const { return failures_; }

 private:
  int failures_ = 0;
};

bool Near(double a, double b, double eps = 1e-9) {
  return std::abs(a - b) < eps;
}

// Section 6.1.1.
void TestForwardPrices(Checker& t) {
  t.Expect(Near(ForwardPrice(100, 0.08, 0.75), 100 * std::exp(0.06)),
           "F = S e^(rT)");
  t.Expect(Near(ForwardPriceDividends(100, 0.08, 0.75, {}),
                ForwardPrice(100, 0.08, 0.75)),
           "no dividends");
  t.Expect(Near(ForwardPriceYield(100, 0.08, 0.0, 0.75),
                ForwardPrice(100, 0.08, 0.75)),
           "zero yield");
  t.Expect(Near(ForwardPriceDividends(100, 0.08, 0.75, {{2, 0.25}, {2, 0.5}}),
                ForwardPrice(100, 0.08, 0.75) - 2 * std::exp(0.04) -
                    2 * std::exp(0.02)),
           "dividends");
  t.Expect(Near(ForwardArbitrageProfit(ForwardPrice(100, 0.08, 0.75), 100, 0.08,
                                       0.75),
                0),
           "fair forward");
}

// Section 6.1.2.
void TestForwardValue(Checker& t) {
  const double f0 = ForwardPrice(100, 0.08, 0.75);
  t.Expect(Near(LongForwardValue(f0, f0, 0.08, 0.25), 0),
           "V = 0 at the same price");
  // A long forward is a long stock plus a short bond paying f0 at T.
  t.Expect(Near(LongForwardValue(f0, ForwardPrice(104, 0.08, 0.25), 0.08, 0.25),
                104 - f0 * std::exp(-0.08 * 0.25)),
           "long forward = stock - bond");
}

// Section 6.2.1.
void TestFutures(Checker& t) {
  const auto tree = FuturesTree(100, 0.10, -0.05, 0.03, 3);
  for (int n = 0; n < 4; ++n) {
    for (int k = 0; k <= n; ++k) {
      t.Expect(Near(tree[n][k], 100 * std::pow(1.1, k) * std::pow(0.95, n - k) *
                                    std::pow(1.03, 3 - n)),
               "futures = forward with constant r");
    }
  }
  const Vec prices{106.0, 104.5, 101.0, 103.5, 99.0, 102.0};
  const auto days = MarginAccount(prices, 100, 1000, 750);
  const double calls[] = {0, 500, 0, 0, 0};
  for (int i = 0; i < 5; ++i)
    t.Expect(Near(days[i].call, calls[i]), "margin calls");
  // Final balance = initial + calls + total gain.
  t.Expect(Near(days.back().balance,
                1000 + 500 + 100 * (prices.back() - prices.front())),
           "final balance");
}

// Section 6.2.2.
void TestHedging(Checker& t) {
  const Vec ds{-6.0, -1.0, 2.0, 5.0};
  const Vec df{-5.0, -2.0, 3.0, 4.0};
  const Vec p{0.25, 0.25, 0.25, 0.25};
  const double h = MinVarianceHedgeRatio(ds, df, p);
  t.Expect(Near(h, 29.0 / 27), "h*");
  t.Expect(HedgedStd(h, ds, df, p) < HedgedStd(h - 0.05, ds, df, p) &&
               HedgedStd(h, ds, df, p) < HedgedStd(h + 0.05, ds, df, p),
           "h* minimises risk");
  t.Expect(Near(HedgedStd(1.0, ds, ds, p), 0), "perfect hedge");
}

int RunAllTests() {
  Checker t;
  TestForwardPrices(t);
  TestForwardValue(t);
  TestFutures(t);
  TestHedging(t);
  if (t.failures() == 0) std::printf("All ch06 tests passed\n");
  return t.failures() == 0 ? 0 : 1;
}

}  // namespace
}  // namespace m4f::ch06

int main() { return m4f::ch06::RunAllTests(); }
