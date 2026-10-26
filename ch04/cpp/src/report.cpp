#include "m4f/report.hpp"
#include "m4f/no_arbitrage.hpp"
#include "m4f/strategies.hpp"

#include <cmath>
#include <format>
#include <ostream>

namespace m4f::ch04 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

std::string fl(const Vec& vs) {
    std::string s = "[";
    for (std::size_t i = 0; i < vs.size(); ++i) s += (i ? ", " : "") + f(vs[i]);
    return s + "]";
}

// Hold one share plus one more for every down move so far.
double contrarian(int, const std::vector<int>& moves) {
    int ups = 0;
    for (int m : moves) ups += m;
    return 1.0 + (static_cast<int>(moves.size()) - ups);
}

}  // namespace

void printReport(std::ostream& os) {
    os << "Chapter 4: Discrete Time Market Models (illustrative numbers, book formulas)\n";

    os << "\n== 4.1.1 Investment strategies: self-financing x(n+1)S(n) + y(n+1)A(n) = V(n) ==\n";
    const Vec S{100.0, 110.0, 104.5, 114.95};
    Vec A;
    for (int n = 0; n < 4; ++n) A.push_back(100.0 * std::pow(1.03, n));
    const Vec x{5.0, 8.0, 3.0};
    const auto [y, V] = selfFinancing(1000.0, S, A, x);
    os << "V(0)=1000, A(n)=100*1.03^n\n";
    for (int n = 1; n < 4; ++n)
        os << std::format("n={}: S={} A={} x={} y={} V={}\n", n, f(S[n]), f(A[n]), f(x[n - 1]), f(y[n - 1]),
                          f(V[n]));
    os << std::format("V(3)-V(0)={} gains sum={}\n", f(V[3] - V[0]), f(gains(S, A, x, y)));
    os << std::format("V~(3)-V~(0)={} discounted gains={}\n", f(V[3] / A[3] - V[0] / A[0]),
                      f(discountedGains(S, A, x)));

    os << "\n== 4.1.2 Principle of no arbitrage: S(0)=100 A(0)=100, K in {0.10, -0.05} ==\n";
    for (double r : {0.12, 0.03, -0.05}) {
        const Vec rets{0.10, -0.05};
        if (isArbitrageFree(rets, r)) {
            os << std::format("r={}: no arbitrage\n", f(r));
        } else {
            const auto [xs, ys] = *arbitrageStrategy(rets, r, 100.0, 100.0);
            os << std::format("r={}: arbitrage x={} y={} V(0)={} V(1)={}\n", f(r), f(xs), f(ys),
                              f(xs * 100.0 + ys * 100.0), fl(oneStepValues(xs, ys, rets, r, 100.0, 100.0)));
        }
    }

    os << "\n== 4.1.3 Binomial tree: S(0)=100 u=0.10 d=-0.05 r=0.03 N=4, x(n) = 1 + #downs so far ==\n";
    const double q = (0.03 - -0.05) / (0.10 - -0.05);
    os << std::format("V(0)=100: E*(V~(4))={} (p*={}), E(V~(4))={} (p=0.6)\n",
                      f(expectedDiscountedValue(100.0, 0.10, -0.05, 0.03, 4, 100.0, contrarian, q)), f(q),
                      f(expectedDiscountedValue(100.0, 0.10, -0.05, 0.03, 4, 100.0, contrarian, 0.6)));

    os << "\n== 4.1.4 Fundamental Theorem: no arbitrage <=> risk-neutral q > 0, K in {-0.10, 0, 0.10} ==\n";
    const Vec rets{-0.10, 0.0, 0.10};
    for (double r : {-0.15, 0.02, 0.05, 0.10}) {
        const auto qs = riskNeutralProbs(rets, r);
        if (!qs) {
            os << std::format("r={}: arbitrage, no risk-neutral probability\n", f(r));
        } else {
            double s = 0.0;
            for (std::size_t i = 0; i < rets.size(); ++i) s += (*qs)[i] * rets[i];
            os << std::format("r={}: q={} sum qK={}\n", f(r), fl(*qs), f(s));
        }
    }

    os << "\n== 4.2 Extended models: two stocks, dividend 2 on stock 2 at n=2, A(n)=1.05^n ==\n";
    const Mat S2{{50.0, 55.0, 52.0, 58.0}, {20.0, 19.0, 21.0, 22.0}};
    Vec A2;
    for (int n = 0; n < 4; ++n) A2.push_back(std::pow(1.05, n));
    const Mat x2{{10.0, 12.0, 8.0}, {20.0, 15.0, 25.0}};
    const Mat div{{0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 2.0, 0.0}};
    const auto p2 = selfFinancingMulti(1000.0, S2, A2, x2, &div);
    for (int n = 1; n < 4; ++n)
        os << std::format("n={}: x1={} x2={} y={} V={}\n", n, f(x2[0][n - 1]), f(x2[1][n - 1]), f(p2.y[n - 1]),
                          f(p2.V[n]));
    const auto nd = selfFinancingMulti(1000.0, S2, A2, x2);
    os << std::format("Without the dividend V(3)={}; it adds {} = 2 x 15 x 1.05\n", f(nd.V[3]),
                      f(p2.V[3] - nd.V[3]));
}

}  // namespace m4f::ch04
