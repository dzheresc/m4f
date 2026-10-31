// Unit tests for chapter 2. A tiny self-contained harness keeps the build free
// of third-party dependencies; each check mirrors a Kotlin and Python test.
#include <cmath>
#include <cstdio>
#include <optional>

#include "m4f/interest.h"
#include "m4f/money_market.h"

namespace m4f::ch02 {
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

void TestLibraryIdentities(Checker& t) {
  t.Expect(Near(SimplePresentValue(SimpleValue(100, 0.1, 0.5), 0.1, 0.5), 100),
           "simple PV");
  t.Expect(Near(SimpleRate(100, SimpleValue(100, 0.08, 2), 2), 0.08),
           "simple rate");
  t.Expect(Near(EffectiveRate(0.1, 1), 0.1), "effective m=1");
  t.Expect(
      Near(PeriodicFromContinuous(ContinuousFromPeriodic(0.1, 12), 12), 0.1),
      "rate roundtrip");
  t.Expect(
      Near(PeriodicFromPeriodic(PeriodicFromPeriodic(0.1, 4, 12), 12, 4), 0.1),
      "m->k->m");
  t.Expect(Near(ContinuousEffectiveRate(ContinuousFromPeriodic(0.1, 1)), 0.1),
           "continuous effective");
  t.Expect(Near(PeriodicValue(100, 0.1, 1000000, 1),
                ContinuousValue(100, 0.1, 1), 1e-4),
           "continuous limit");
  t.Expect(
      Near(AnnuityPresentValue(LoanInstallment(1000, 0.1, 5), 0.1, 5), 1000),
      "loan");
  t.Expect(Near(AnnuityFutureValue(1, 0.1, 5),
                AnnuityFactor(0.1, 5) * std::pow(1.1, 5)),
           "FV");
  for (std::optional<int> m : {std::optional<int>{1}, std::optional<int>{2},
                               std::optional<int>{12}, std::optional<int>{}}) {
    t.Expect(Near(ImpliedRate(ZeroCouponPrice(0.08, 2.5, m), 2.5, m), 0.08),
             "implied rate");
  }
  t.Expect(Near(YieldToMaturity(CouponBondPrice(100, 10, 3, 0.12), 100, 10, 3),
                0.12, 1e-8),
           "YTM");
  t.Expect(Near(CouponBondPrice(100, 10, 3, 0.10), 100), "par bond");
}

void TestReportValues(Checker& t) {
  t.Expect(Near(AnnuityPresentValue(1000, 0.10, 10), 6144.567105, 1e-6),
           "annuity PV");
  t.Expect(Near(LoanInstallment(100000, 0.005, 300), 644.301401, 1e-6),
           "mortgage installment");
  t.Expect(Near(ContinuousFromPeriodic(0.12, 12), 0.119404, 1e-6),
           "12% monthly -> continuous");
  t.Expect(Near(MoneyMarketRollover(1, {0.5, 0.5}), 4), "rollover");
}

int RunAllTests() {
  Checker t;
  TestLibraryIdentities(t);
  TestReportValues(t);
  if (t.failures() == 0) {
    std::printf("all tests passed\n");
    return 0;
  }
  std::printf("%d failure(s)\n", t.failures());
  return 1;
}

}  // namespace
}  // namespace m4f::ch02

int main() { return m4f::ch02::RunAllTests(); }
