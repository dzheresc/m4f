#pragma once
// Section 4.1, stock and money market models: investment strategies.
//
// Prices are given along one path: S[n] (or S[j][n] for several stocks) and A[n]
// for n = 0..N. A strategy holds x(n) shares and y(n) units of the money market
// over the step from n-1 to n, chosen at time n-1; vectors are indexed from 0,
// so x[0] is x(1).
#include <cstddef>
#include <vector>

namespace m4f::ch04 {

using Vec = std::vector<double>;
using Mat = std::vector<Vec>;

struct StrategyPath {
    Vec y;  // y[n-1] = y(n)
    Vec V;  // V[n] = V(n)
};

// Several stocks S[j][n], optional dividends div[j][n] paid at n to the holders
// over (n-1, n]. The money market position y(n+1) is set so that rebalancing at
// n costs exactly V(n):
//   sum_j x_j(n+1) S_j(n) + y(n+1) A(n) = V(n),
//   V(n) = sum_j x_j(n) (S_j(n) + div_j(n)) + y(n) A(n).
inline StrategyPath selfFinancingMulti(double V0, const Mat& S, const Vec& A, const Mat& x,
                                       const Mat* div = nullptr) {
    const std::size_t N = A.size() - 1;
    StrategyPath p{{}, {V0}};
    for (std::size_t n = 1; n <= N; ++n) {
        double stock = 0.0;
        for (std::size_t j = 0; j < S.size(); ++j) stock += x[j][n - 1] * S[j][n - 1];
        p.y.push_back((p.V[n - 1] - stock) / A[n - 1]);
        double value = p.y[n - 1] * A[n];
        for (std::size_t j = 0; j < S.size(); ++j) {
            const double d = div ? (*div)[j][n] : 0.0;
            value += x[j][n - 1] * (S[j][n] + d);
        }
        p.V.push_back(value);
    }
    return p;
}

// One stock: y(n) and V(n) of the self-financing strategy with initial value V0
// and stock positions x.
inline StrategyPath selfFinancing(double V0, const Vec& S, const Vec& A, const Vec& x) {
    return selfFinancingMulti(V0, Mat{S}, A, Mat{x});
}

// V(N) - V(0) = sum_n x(n) (S(n) - S(n-1)) + y(n) (A(n) - A(n-1))
inline double gains(const Vec& S, const Vec& A, const Vec& x, const Vec& y) {
    double g = 0.0;
    for (std::size_t n = 1; n < A.size(); ++n)
        g += x[n - 1] * (S[n] - S[n - 1]) + y[n - 1] * (A[n] - A[n - 1]);
    return g;
}

// V~(N) - V~(0) = sum_n x(n) (S~(n) - S~(n-1)), with S~(n) = S(n) / A(n)
inline double discountedGains(const Vec& S, const Vec& A, const Vec& x) {
    double g = 0.0;
    for (std::size_t n = 1; n < A.size(); ++n) g += x[n - 1] * (S[n] / A[n] - S[n - 1] / A[n - 1]);
    return g;
}

}  // namespace m4f::ch04
