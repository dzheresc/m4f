// Unit tests for chapter 3. A tiny self-contained harness keeps the build free
// of third-party dependencies; each check mirrors a Kotlin and Python test.
#include <cmath>
#include <cstdio>
#include <vector>

#include "m4f/binomial_tree.h"
#include "m4f/other_models.h"
#include "m4f/returns.h"

namespace m4f::ch03 {
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

void TestReportValues(Checker& t) {
  const BinomialTree tree{100, 0.10, -0.05, 0.03};
  t.Expect(Near(tree.ExpectedPrice(3, 0.6), 112.4864), "E(S(3))");
  t.Expect(Near(tree.RiskNeutralProb(), 8.0 / 15), "p*");
  t.Expect(Near(TrinomialModel{0.10, 0.0, -0.10, 0.02}.MaxMiddleProb(), 0.8),
           "p_m bound");
  t.Expect(
      Near(LognormalExpectedPrice(100, 0.05, 0.2, 1), 100 * std::exp(0.07)),
      "lognormal mean");
}

// Section 3.1.
void TestReturns(Checker& t) {
  const std::vector<double> path{45.0, 48.0, 47.5, 50.0};
  t.Expect(Near(CompoundReturn(Returns(path)), OneStepReturn(45, 50)),
           "compound return");
  const std::vector<double> log_returns = LogReturns(path);
  t.Expect(
      Near(log_returns[0] + log_returns[1] + log_returns[2], LogReturn(45, 50)),
      "log returns add");
  t.Expect(Near(Expectation({1, 3}, {0.5, 0.5}), 2), "expectation");
  t.Expect(Near(Variance({1, 3}, {0.5, 0.5}), 1), "variance");
}

// Section 3.2.
void TestBinomialTree(Checker& t) {
  double total = 0;
  for (int k = 0; k <= 5; ++k) total += BinomialProb(5, k, 0.3);
  t.Expect(Near(total, 1), "binomial probabilities sum to 1");
  const BinomialTree tree{60, 0.2, -0.1, 0.05};
  const double q = tree.RiskNeutralProb();
  t.Expect(tree.IsArbitrageFree(), "d < r < u");
  t.Expect(Near(tree.ExpectedReturn(q), 0.05), "E*(K) = r");
  for (int n = 0; n < 4; ++n) {
    t.Expect(Near(tree.ExpectedPrice(n, q), 60 * std::pow(1.05, n)),
             "E*(S(n))");
    t.Expect(Near(tree.ExpectedPrice(n, 0.4),
                  60 * std::pow(1 + tree.ExpectedReturn(0.4), n)),
             "E(S(n))");
    for (int k = 0; k <= n; ++k) {
      t.Expect(
          Near(tree.ConditionalDiscounted(n, k), tree.DiscountedPrice(n, k)),
          "martingale");
    }
  }
  t.Expect(!BinomialTree{60, 0.2, 0.1, 0.05}.IsArbitrageFree(),
           "arbitrage when r < d");
}

// Section 3.3.1.
void TestTrinomial(Checker& t) {
  const TrinomialModel model{0.10, 0.0, -0.10, 0.02};
  for (double p_mid : {0.1, 0.5, 0.79}) {
    const RiskNeutralProbs p = model.GetRiskNeutralProbs(p_mid);
    t.Expect(Near(p.p_up + p.p_mid + p.p_down, 1), "trinomial sum");
    t.Expect(Near(p.p_up * 0.10 + p.p_down * -0.10, 0.02),
             "trinomial E*(K) = r");
    t.Expect(p.p_up > 0 && p.p_down > 0, "trinomial positive");
  }
  t.Expect(model.GetRiskNeutralProbs(0.81).p_down < 0, "p_m above bound");
}

// Section 3.3.2.
void TestContinuousLimit(Checker& t) {
  t.Expect(Near(NormCdf(0), 0.5, 1e-7), "Phi(0)");
  t.Expect(Near(NormCdf(1.96), 0.9750021, 1e-6), "Phi(1.96)");
  t.Expect(Near(NormCdf(-1) + NormCdf(1), 1), "Phi symmetry");
  t.Expect(Near(BinomialExpectedPrice(100, 0.05, 0.2, 1, 100000),
                LognormalExpectedPrice(100, 0.05, 0.2, 1), 1e-4),
           "E(S(T)) limit");
  t.Expect(Near(BinomialProbAbove(100, 0.05, 0.2, 1, 20000, 110),
                LognormalProbAbove(100, 0.05, 0.2, 1, 110), 1e-2),
           "P(S(T)>K) limit");
  t.Expect(Near(BinomialProbAbove(100, 0.0, 0.2, 1, 7, 1e-9), 1),
           "P(S(T)>0) = 1");
}

int RunAllTests() {
  Checker t;
  TestReportValues(t);
  TestReturns(t);
  TestBinomialTree(t);
  TestTrinomial(t);
  TestContinuousLimit(t);
  if (t.failures() == 0) {
    std::printf("All ch03 tests passed\n");
    return 0;
  }
  return 1;
}

}  // namespace
}  // namespace m4f::ch03

int main() { return m4f::ch03::RunAllTests(); }
