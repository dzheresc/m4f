// Unit tests for chapter 4. A tiny self-contained harness keeps the build free
// of third-party dependencies; each check mirrors a Kotlin and Python test.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <optional>
#include <vector>

#include "m4f/no_arbitrage.h"
#include "m4f/strategies.h"

namespace m4f::ch04 {
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

int CountUps(const std::vector<int>& moves) {
  int ups = 0;
  for (int move : moves) ups += move;
  return ups;
}

// Section 4.1.1.
void TestSelfFinancing(Checker& t) {
  const Vec s{100.0, 110.0, 104.5, 114.95};
  Vec a;
  for (int n = 0; n < 4; ++n) a.push_back(100.0 * std::pow(1.03, n));
  const Vec x{5.0, 8.0, 3.0};
  const auto [y, values] = SelfFinancing(1000.0, s, a, x);
  t.Expect(Near(values[3], 1079.2915, 1e-4), "V(3)");
  for (int n = 1; n < 3; ++n) {
    t.Expect(Near(x[n] * s[n] + y[n] * a[n], values[n]),
             "rebalancing costs V(n)");
  }
  t.Expect(Near(Gains(s, a, x, y), values[3] - values[0]), "gains");
  t.Expect(Near(DiscountedGains(s, a, x), values[3] / a[3] - values[0] / a[0]),
           "discounted gains");
}

// Section 4.2.
void TestDividends(Checker& t) {
  const Mat s{{50.0, 55.0, 52.0, 58.0}, {20.0, 19.0, 21.0, 22.0}};
  Vec a;
  for (int n = 0; n < 4; ++n) a.push_back(std::pow(1.05, n));
  const Mat x{{10.0, 12.0, 8.0}, {20.0, 15.0, 25.0}};
  const Mat dividends{{0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 2.0, 0.0}};
  t.Expect(Near(SelfFinancingMulti(1000.0, s, a, x, &dividends).values[3] -
                    SelfFinancingMulti(1000.0, s, a, x).values[3],
                2 * 15 * 1.05),
           "dividend");
}

// Section 4.1.2.
void TestArbitrage(Checker& t) {
  const Vec returns{0.10, -0.05};
  t.Expect(IsArbitrageFree(returns, 0.03) &&
               !ArbitrageStrategy(returns, 0.03, 100, 100),
           "no arbitrage");
  for (double r : {0.10, 0.12, -0.05, -0.08}) {
    const auto [x, y] = *ArbitrageStrategy(returns, r, 100, 100);
    t.Expect(Near(x * 100 + y * 100, 0), "V(0) = 0");
    const Vec v1 = OneStepValues(x, y, returns, r, 100, 100);
    t.Expect(std::min(v1[0], v1[1]) >= -1e-12 && std::max(v1[0], v1[1]) > 0,
             "V(1) >= 0, > 0 somewhere");
  }
  t.Expect(Near((*RiskNeutralProbs(returns, 0.03))[0], 8.0 / 15),
           "binomial p*");
}

// Section 4.1.4.
void TestFundamentalTheorem(Checker& t) {
  for (const Vec& returns :
       {Vec{0.10, -0.05}, Vec{-0.10, 0.0, 0.10}, Vec{0.3, -0.2, 0.05, 0.1}}) {
    for (double r : {-0.3, -0.09, 0.0, 0.04, 0.09, 0.5}) {
      const std::optional<Vec> q = RiskNeutralProbs(returns, r);
      t.Expect(q.has_value() == IsArbitrageFree(returns, r), "FTAP");
      if (!q) continue;
      double sum = 0;
      double mean = 0;
      double smallest = 1;
      for (std::size_t i = 0; i < returns.size(); ++i) {
        sum += (*q)[i];
        mean += (*q)[i] * returns[i];
        smallest = std::min(smallest, (*q)[i]);
      }
      t.Expect(Near(sum, 1) && Near(mean, r) && smallest > 0, "risk-neutral q");
    }
  }
}

// Section 4.1.3: martingale property of discounted self-financing values.
void TestMartingale(Checker& t) {
  const double q = 8.0 / 15;
  const Rule rules[] = {
      [](int, const std::vector<int>& moves) {
        return 1.0 + (static_cast<int>(moves.size()) - CountUps(moves));
      },
      [](int, const std::vector<int>& moves) {
        return 3.0 * CountUps(moves) - 2.0;
      },
      [](int n, const std::vector<int>&) { return static_cast<double>(n); },
  };
  for (const Rule& rule : rules) {
    for (double v0 : {0.0, 100.0}) {
      t.Expect(
          Near(ExpectedDiscountedValue(100, 0.10, -0.05, 0.03, 4, v0, rule, q),
               v0),
          "E*(V~(N)) = V(0)");
    }
  }
}

int RunAllTests() {
  Checker t;
  TestSelfFinancing(t);
  TestDividends(t);
  TestArbitrage(t);
  TestFundamentalTheorem(t);
  TestMartingale(t);
  if (t.failures() == 0) {
    std::printf("All ch04 tests passed\n");
    return 0;
  }
  return 1;
}

}  // namespace
}  // namespace m4f::ch04

int main() { return m4f::ch04::RunAllTests(); }
