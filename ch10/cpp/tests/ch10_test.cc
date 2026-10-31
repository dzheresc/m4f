// Unit tests for chapter 10. A tiny self-contained harness keeps the build
// free of third-party dependencies; each check mirrors a Kotlin and Python
// test.
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <utility>
#include <vector>

#include "m4f/bonds.h"
#include "m4f/term_structure.h"

namespace m4f::ch10 {
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

bool Near(double a, double b, double eps = 1e-7) {
  return std::abs(a - b) < eps;
}

// Sections 10.1.1-10.1.2.
void TestSingleBonds(Checker& t) {
  const Bond z = ZeroCoupon(100.0, 4.0);
  t.Expect(Near(Price(z, 0.06), 100 * std::exp(-0.24)), "zero price");
  t.Expect(Near(Duration(z, 0.06), 4.0) && Near(Duration(z, 0.06, 1.5), 2.5),
           "zero duration");
  const Bond b = CouponBond(100.0, 5.0, 4);
  t.Expect(b.size() == 4 && b[3].t == 4.0 && b[3].amount == 105.0 &&
               b[0].amount == 5.0,
           "coupon bond flows");
  const double h = 1e-5;
  t.Expect(Near((Price(b, 0.06 + h) - Price(b, 0.06 - h)) / (2 * h),
                -Duration(b, 0.06) * Price(b, 0.06), 1e-4),
           "dP/dy = -DP");
  t.Expect(Duration(b, 0.06) < 4.0, "D < maturity");

  for (double horizon : {1.0, 2.5, 4.0}) {
    t.Expect(Near(InvestmentValue(b, 0.06, horizon),
                  Price(b, 0.06) * std::exp(0.06 * horizon)),
             "unchanged yield grows at y");
    t.Expect(Near(LogReturn(b, 0.06, 0.06, horizon), 0.06), "log return = y");
  }
  const double d = Duration(b, 0.06);
  t.Expect(std::abs(InvestmentValue(b, 0.061, d) -
                    InvestmentValue(b, 0.06, d)) < 1e-2,
           "immunised at t=D");
  t.Expect(InvestmentValue(b, 0.07, 1.0) < InvestmentValue(b, 0.06, 1.0),
           "short horizon loses when y rises");
  t.Expect(InvestmentValue(b, 0.07, 4.0) > InvestmentValue(b, 0.06, 4.0),
           "long horizon gains when y rises");
}

// Sections 10.1.3-10.1.4.
void TestImmunisation(Checker& t) {
  const Bond a = CouponBond(100.0, 4.0, 5);
  const Bond b = CouponBond(100.0, 8.0, 10);
  const auto [w1, w2] =
      ImmunisingWeights(Duration(a, 0.06), Duration(b, 0.06), 5.0);
  t.Expect(Near(w1 + w2, 1.0) &&
               Near(w1 * Duration(a, 0.06) + w2 * Duration(b, 0.06), 5.0),
           "immunising weights");
  const Portfolio p = Immunise(1000.0, a, b, 0.06, 0.0, 5.0);
  t.Expect(Near(PortfolioValue(p, a, b, 0.06, 0.0), 1000.0),
           "immunise invests V");
  const double target = 1000.0 * std::exp(0.06 * 5);
  for (double y : {0.05, 0.07}) {
    t.Expect(
        p.q1 * InvestmentValue(a, y, 5.0) + p.q2 * InvestmentValue(b, y, 5.0) >
            target - 1e-9,
        "no shortfall");
  }

  std::vector<std::pair<double, double>> flat;
  for (int i = 0; i <= 10; ++i) flat.push_back({0.5 * i, 0.06});
  const auto steps = DynamicHedge(1000.0, a, b, 5.0, flat);
  t.Expect(steps.size() == 11 &&
               Near(steps.back().value, 1000.0 * std::exp(0.3), 1e-6),
           "flat path reaches target");
  for (std::size_t i = 0; i + 1 < steps.size(); ++i) {
    t.Expect(Near(steps[i].duration, 5.0 - steps[i].t, 1e-9),
             "D = remaining horizon");
  }
  t.Expect(Near(DynamicHedge(1000.0, a, b, 5.0, flat, false).back().value,
                1000.0 * std::exp(0.3), 1e-6),
           "static flat path");
}

// Section 10.2.
void TestTermStructure(Checker& t) {
  const std::vector<double> maturities = {0.5, 1.0, 1.5, 2.0, 2.5, 3.0};
  const std::vector<double> prices = {0.978, 0.953, 0.926, 0.898, 0.870, 0.842};
  const auto forwards = ForwardRates(maturities, prices);
  t.Expect(Near(forwards[0], YieldRate(prices[0], 0.5)),
           "first forward = yield");
  const auto rebuilt = PricesFromForwardRates(maturities, forwards);
  for (std::size_t i = 0; i < maturities.size(); ++i) {
    t.Expect(Near(rebuilt[i], prices[i]), "prices from forwards");
  }
  t.Expect(Near(ForwardRate(prices[1], 1.0, prices[5], 3.0),
                (forwards[2] + forwards[3] + forwards[4] + forwards[5]) / 4),
           "forward average");
  t.Expect(Near(ImpliedPrice(prices[1], prices[3]),
                std::exp(-ForwardRate(prices[1], 1.0, prices[3], 2.0))),
           "implied price");
  const auto account = MoneyMarketAccount(maturities, prices);
  for (std::size_t i = 0; i < maturities.size(); ++i) {
    t.Expect(Near(account[i], 1 / prices[i]), "A = 1/B");
  }
  const double p =
      CouponBondPrice({1.0, 2.0, 3.0}, {6.0, 6.0, 106.0}, maturities, prices);
  t.Expect(Near(p, 6 * prices[1] + 6 * prices[3] + 106 * prices[5]),
           "coupon bond price");
  const double y = YieldToMaturity(p, {1.0, 2.0, 3.0}, {6.0, 6.0, 106.0});
  t.Expect(
      Near(6 * std::exp(-y) + 6 * std::exp(-2 * y) + 106 * std::exp(-3 * y), p,
           1e-8),
      "ytm");
  t.Expect(Near(YieldToMaturity(prices[5] * 100, {3.0}, {100.0}),
                YieldRate(prices[5], 3.0), 1e-9),
           "ytm of zero");
}

// The faster CouponBondPrice, YieldToMaturity and Duration against the
// straightforward versions they replaced.
void TestAgainstReference(Checker& t) {
  auto ref_price =
      [](const std::vector<double>& ft, const std::vector<double>& fa,
         const std::vector<double>& mat, const std::vector<double>& pr) {
        double total = 0.0;
        for (std::size_t i = 0; i < ft.size(); ++i) {
          for (std::size_t j = 0; j < mat.size(); ++j) {
            if (mat[j] == ft[i]) {
              total += fa[i] * pr[j];
              break;
            }
          }
        }
        return total;
      };
  auto ref_ytm = [](double price, const std::vector<double>& ft,
                    const std::vector<double>& fa) {
    double lo = -1.0, hi = 1.0;
    for (int it = 0; it < 200; ++it) {
      const double mid = (lo + hi) / 2;
      double s = 0.0;
      for (std::size_t i = 0; i < ft.size(); ++i) {
        s += fa[i] * std::exp(-mid * ft[i]);
      }
      (s > price ? lo : hi) = mid;
    }
    return (lo + hi) / 2;
  };
  std::vector<double> mat, pr;
  for (int k = 1; k <= 120; ++k) {
    mat.push_back(k / 12.0);
    pr.push_back(std::exp(-0.04 * k / 12.0));
  }
  // Sorted, unsorted, off-grid and repeated flow times.
  const std::vector<std::vector<double>> times = {{1 / 12.0, 0.5, 1.0, 10.0},
                                                  {10.0, 1.0, 0.5, 1 / 12.0},
                                                  {0.3, 1.0, 11.0, 2.0, 2.0}};
  for (const auto& ft : times) {
    std::vector<double> fa(ft.size(), 5.0);
    fa.back() = 105.0;
    t.Expect(CouponBondPrice(ft, fa, mat, pr) == ref_price(ft, fa, mat, pr),
             "CouponBondPrice matches linear search");
  }
  for (int n : {1, 3, 30, 360}) {
    std::vector<double> ft, fa;
    for (int i = 1; i <= n; ++i) {
      ft.push_back(i / 12.0);
      fa.push_back(i == n ? 100.5 : 0.5);
    }
    // Yields inside the bracket, plus prices whose root lies outside it.
    for (double price : {50.0, 80.0, 100.0, 101.0, 130.0, 1.0, 1e6}) {
      const double y = YieldToMaturity(price, ft, fa);
      const double ref = ref_ytm(price, ft, fa);
      t.Expect(std::abs(y - ref) <= 1e-13, "YieldToMaturity matches bisection");
    }
  }
  const Bond bond = CouponBond(100.0, 6.0, 10, 0.5);
  for (double tt : {0.0, 0.7, 4.9}) {
    double s = 0.0;
    for (const Flow& flow : bond) {
      if (flow.t > tt) {
        s += (flow.t - tt) * flow.amount * std::exp(-0.05 * (flow.t - tt));
      }
    }
    t.Expect(Duration(bond, 0.05, tt) == s / Price(bond, 0.05, tt),
             "Duration bit-identical");
  }
}

int RunAllTests() {
  Checker t;
  TestSingleBonds(t);
  TestImmunisation(t);
  TestTermStructure(t);
  TestAgainstReference(t);
  if (t.failures() == 0) std::printf("All C++ tests passed\n");
  return t.failures() == 0 ? 0 : 1;
}

}  // namespace
}  // namespace m4f::ch10

int main() { return m4f::ch10::RunAllTests(); }
