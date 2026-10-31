// Unit tests for chapter 7. A tiny self-contained harness keeps the build free
// of third-party dependencies; each check mirrors a Kotlin and Python test.
#include <cmath>
#include <cstdio>

#include "m4f/binomial.h"
#include "m4f/options.h"

namespace m4f::ch07 {
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

double Price(double s0, double strike, double maturity, bool is_call,
             bool american = false) {
  return CrrPrice(s0, strike, 0.05, 0.2, maturity, 200, is_call, american);
}

// Section 7.1.
void TestPayoffs(Checker& t) {
  for (double s : {80.0, 100.0, 120.0}) {
    t.Expect(Near(CallPayoff(s, 100) - PutPayoff(s, 100), s - 100), "payoffs");
  }
  t.Expect(Near(Profit(0.0, 10.0, 0.05, 1.0), -10 * std::exp(0.05)), "profit");
}

// Section 7.2.
void TestParity(Checker& t) {
  t.Expect(Near(ParityGap(Price(100, 100, 1, true), Price(100, 100, 1, false),
                          100, 100, 0.05, 1),
                0),
           "parity");
  t.Expect(Near(ParityGap(12.0, 5.0, 100, 100, 0.05, 1),
                7 - 100 + 100 * std::exp(-0.05)),
           "parity gap");
}

// Section 7.3.
void TestBounds(Checker& t) {
  for (double s0 : {80.0, 100.0, 120.0}) {
    for (double maturity : {0.25, 1.0}) {
      t.Expect(BoundViolation(Price(s0, 100, maturity, true),
                              EuropeanCallBounds(s0, 100, 0.05, maturity)) == 0,
               "call bounds");
      t.Expect(BoundViolation(Price(s0, 100, maturity, false),
                              EuropeanPutBounds(s0, 100, 0.05, maturity)) == 0,
               "put bounds");
      const Bounds b = AmericanParityBounds(s0, 100, 0.05, maturity);
      const double d = Price(s0, 100, maturity, true, true) -
                       Price(s0, 100, maturity, false, true);
      t.Expect(b.lo <= d && d <= b.hi, "American parity bounds");
    }
  }
  t.Expect(Near(BoundViolation(3.5, {4.0, 100.0}), 0.5) &&
               Near(BoundViolation(101.0, {4.0, 100.0}), 1.0),
           "violation");
}

// Sections 7.3.2 and 7.4.2.
void TestAmerican(Checker& t) {
  for (double s0 : {80.0, 100.0, 120.0}) {
    t.Expect(Near(Price(s0, 100, 1, true, true), Price(s0, 100, 1, true)),
             "C_A = C_E");
    t.Expect(Price(s0, 100, 1, false, true) >= Price(s0, 100, 1, false),
             "P_A >= P_E");
    t.Expect(Price(s0, 100, 1, false, true) >= PutPayoff(s0, 100),
             "P_A >= payoff");
  }
  double prev = 0;
  for (double maturity : {0.25, 0.5, 1.0, 2.0}) {
    const double p = Price(100, 100, maturity, false, true);
    t.Expect(p >= prev, "P_A increasing in T");
    prev = p;
  }
}

// Section 7.4.1.
void TestStrike(Checker& t) {
  const double c1 = Price(100, 90, 1, true);
  const double c2 = Price(100, 100, 1, true);
  const double c3 = Price(100, 110, 1, true);
  t.Expect(c1 > c2 && c2 > c3, "C decreasing in X");
  t.Expect(ButterflyGap(c1, c2, c3, 90, 100, 110) <= 0, "convex in X");
  t.Expect(c1 - c2 <= 10 * std::exp(-0.05), "slope in X");
  t.Expect(Near(ButterflyGap(15.0, 10.0, 2.0, 90, 100, 110), 1.5),
           "butterfly arbitrage");
}

int RunAllTests() {
  Checker t;
  TestPayoffs(t);
  TestParity(t);
  TestBounds(t);
  TestAmerican(t);
  TestStrike(t);
  if (t.failures() == 0) std::printf("All ch07 tests passed\n");
  return t.failures() == 0 ? 0 : 1;
}

}  // namespace
}  // namespace m4f::ch07

int main() { return m4f::ch07::RunAllTests(); }
