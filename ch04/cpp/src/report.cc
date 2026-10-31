#include "m4f/report.h"

#include <cmath>
#include <cstddef>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "m4f/no_arbitrage.h"
#include "m4f/strategies.h"

namespace m4f::ch04 {
namespace {

// Four decimals, round-half-even on the exact double. Adding 0.0 turns -0.0
// into 0.0 so that no "-0.0000" appears.
std::string Fmt(double v) { return std::format("{:.4f}", v + 0.0); }

// "[a, b, c]" with each entry formatted by Fmt().
std::string FmtList(const Vec& values) {
  std::string out = "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    out += (i ? ", " : "") + Fmt(values[i]);
  }
  return out + "]";
}

// Hold one share plus one more for every down move so far.
double Contrarian(int /*n*/, const std::vector<int>& moves) {
  int ups = 0;
  for (int move : moves) ups += move;
  return 1.0 + (static_cast<int>(moves.size()) - ups);
}

void PrintStrategies(std::ostream& os) {
  os << "\n== 4.1.1 Investment strategies: self-financing "
        "x(n+1)S(n) + y(n+1)A(n) = V(n) ==\n";
  const Vec s{100.0, 110.0, 104.5, 114.95};
  Vec a;
  for (int n = 0; n < 4; ++n) a.push_back(100.0 * std::pow(1.03, n));
  const Vec x{5.0, 8.0, 3.0};
  const auto [y, values] = SelfFinancing(1000.0, s, a, x);
  os << "V(0)=1000, A(n)=100*1.03^n\n";
  for (int n = 1; n < 4; ++n) {
    os << std::format("n={}: S={} A={} x={} y={} V={}\n", n, Fmt(s[n]),
                      Fmt(a[n]), Fmt(x[n - 1]), Fmt(y[n - 1]), Fmt(values[n]));
  }
  os << std::format("V(3)-V(0)={} gains sum={}\n", Fmt(values[3] - values[0]),
                    Fmt(Gains(s, a, x, y)));
  os << std::format("V~(3)-V~(0)={} discounted gains={}\n",
                    Fmt(values[3] / a[3] - values[0] / a[0]),
                    Fmt(DiscountedGains(s, a, x)));
}

void PrintNoArbitrage(std::ostream& os) {
  os << "\n== 4.1.2 Principle of no arbitrage: S(0)=100 A(0)=100, "
        "K in {0.10, -0.05} ==\n";
  for (double r : {0.12, 0.03, -0.05}) {
    const Vec returns{0.10, -0.05};
    if (IsArbitrageFree(returns, r)) {
      os << std::format("r={}: no arbitrage\n", Fmt(r));
    } else {
      const auto [x, y] = *ArbitrageStrategy(returns, r, 100.0, 100.0);
      os << std::format("r={}: arbitrage x={} y={} V(0)={} V(1)={}\n", Fmt(r),
                        Fmt(x), Fmt(y), Fmt(x * 100.0 + y * 100.0),
                        FmtList(OneStepValues(x, y, returns, r, 100.0, 100.0)));
    }
  }
}

void PrintBinomialTree(std::ostream& os) {
  os << "\n== 4.1.3 Binomial tree: S(0)=100 u=0.10 d=-0.05 r=0.03 N=4, "
        "x(n) = 1 + #downs so far ==\n";
  const double q = (0.03 - -0.05) / (0.10 - -0.05);
  os << std::format("V(0)=100: E*(V~(4))={} (p*={}), E(V~(4))={} (p=0.6)\n",
                    Fmt(ExpectedDiscountedValue(100.0, 0.10, -0.05, 0.03, 4,
                                                100.0, Contrarian, q)),
                    Fmt(q),
                    Fmt(ExpectedDiscountedValue(100.0, 0.10, -0.05, 0.03, 4,
                                                100.0, Contrarian, 0.6)));
}

void PrintFundamentalTheorem(std::ostream& os) {
  os << "\n== 4.1.4 Fundamental Theorem: no arbitrage <=> risk-neutral q > 0, "
        "K in {-0.10, 0, 0.10} ==\n";
  const Vec returns{-0.10, 0.0, 0.10};
  for (double r : {-0.15, 0.02, 0.05, 0.10}) {
    const std::optional<Vec> q = RiskNeutralProbs(returns, r);
    if (!q) {
      os << std::format("r={}: arbitrage, no risk-neutral probability\n",
                        Fmt(r));
    } else {
      double sum = 0.0;
      for (std::size_t i = 0; i < returns.size(); ++i) {
        sum += (*q)[i] * returns[i];
      }
      os << std::format("r={}: q={} sum qK={}\n", Fmt(r), FmtList(*q),
                        Fmt(sum));
    }
  }
}

void PrintExtendedModels(std::ostream& os) {
  os << "\n== 4.2 Extended models: two stocks, dividend 2 on stock 2 at n=2, "
        "A(n)=1.05^n ==\n";
  const Mat s{{50.0, 55.0, 52.0, 58.0}, {20.0, 19.0, 21.0, 22.0}};
  Vec a;
  for (int n = 0; n < 4; ++n) a.push_back(std::pow(1.05, n));
  const Mat x{{10.0, 12.0, 8.0}, {20.0, 15.0, 25.0}};
  const Mat dividends{{0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 2.0, 0.0}};
  const StrategyPath with_dividend =
      SelfFinancingMulti(1000.0, s, a, x, &dividends);
  for (int n = 1; n < 4; ++n) {
    os << std::format("n={}: x1={} x2={} y={} V={}\n", n, Fmt(x[0][n - 1]),
                      Fmt(x[1][n - 1]), Fmt(with_dividend.y[n - 1]),
                      Fmt(with_dividend.values[n]));
  }
  const StrategyPath without = SelfFinancingMulti(1000.0, s, a, x);
  os << std::format(
      "Without the dividend V(3)={}; it adds {} = 2 x 15 x 1.05\n",
      Fmt(without.values[3]), Fmt(with_dividend.values[3] - without.values[3]));
}

}  // namespace

void PrintReport(std::ostream& os) {
  os << "Chapter 4: Discrete Time Market Models (illustrative numbers, book "
        "formulas)\n";
  PrintStrategies(os);
  PrintNoArbitrage(os);
  PrintBinomialTree(os);
  PrintFundamentalTheorem(os);
  PrintExtendedModels(os);
}

}  // namespace m4f::ch04
