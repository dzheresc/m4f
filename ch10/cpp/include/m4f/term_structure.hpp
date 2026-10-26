#pragma once
// Section 10.2: general term structure. The market gives zero-coupon bond
// prices B(0,T) for a grid of maturities T_1 < ... < T_n; all rates are
// continuously compounded. In a deterministic term structure the implied
// forward rates are the future short rates, and B(t,T) = B(0,T)/B(0,t).
#include <cmath>
#include <vector>

namespace m4f::ch10 {

// y(0,T) = -ln B(0,T) / T
inline double yieldRate(double B, double T) { return -std::log(B) / T; }

// f(0,T1,T2) = (ln B(0,T1) - ln B(0,T2)) / (T2 - T1); B(0,0) = 1 gives y(0,T2).
inline double forwardRate(double B1, double T1, double B2, double T2) { return (std::log(B1) - std::log(B2)) / (T2 - T1); }

// f(0,T_{i-1},T_i) for each i, with T_0 = 0.
inline std::vector<double> forwardRates(const std::vector<double>& maturities, const std::vector<double>& prices) {
    std::vector<double> out;
    double Bp = 1.0, Tp = 0.0;
    for (std::size_t i = 0; i < maturities.size(); ++i) {
        out.push_back(forwardRate(Bp, Tp, prices[i], maturities[i]));
        Bp = prices[i];
        Tp = maturities[i];
    }
    return out;
}

// B(0,T_n) = exp(-sum_{i<=n} f(0,T_{i-1},T_i)(T_i - T_{i-1}))
inline std::vector<double> pricesFromForwardRates(const std::vector<double>& maturities, const std::vector<double>& f) {
    std::vector<double> out;
    double acc = 0.0, Tp = 0.0;
    for (std::size_t i = 0; i < maturities.size(); ++i) {
        acc += f[i] * (maturities[i] - Tp);
        out.push_back(std::exp(-acc));
        Tp = maturities[i];
    }
    return out;
}

// B(t,T) implied by no arbitrage in a deterministic term structure: B(0,T)/B(0,t).
inline double impliedPrice(double Bt, double BT) { return BT / Bt; }

// Price of a coupon bond = sum C_i B(0,t_i), with t_i on the grid.
inline double couponBondPrice(const std::vector<double>& flowsT, const std::vector<double>& flowsC,
                              const std::vector<double>& maturities, const std::vector<double>& prices) {
    double total = 0.0;
    for (std::size_t i = 0; i < flowsT.size(); ++i)
        for (std::size_t j = 0; j < maturities.size(); ++j)
            if (maturities[j] == flowsT[i]) { total += flowsC[i] * prices[j]; break; }
    return total;
}

// The single continuous yield y with sum C_i e^(-y t_i) = P (bisection).
inline double yieldToMaturity(double P, const std::vector<double>& flowsT, const std::vector<double>& flowsC) {
    double lo = -1.0, hi = 1.0;
    for (int it = 0; it < 200; ++it) {
        const double mid = (lo + hi) / 2;
        double s = 0.0;
        for (std::size_t i = 0; i < flowsT.size(); ++i) s += flowsC[i] * std::exp(-mid * flowsT[i]);
        if (s > P) lo = mid; else hi = mid;
    }
    return (lo + hi) / 2;
}

// 10.2.2: A(T_n) from rolling over one-period bonds at the (implied, hence
// realised) short rates: A(T_n) = prod 1/B(T_{i-1},T_i) = 1/B(0,T_n).
inline std::vector<double> moneyMarketAccount(const std::vector<double>& /*maturities*/, const std::vector<double>& prices) {
    std::vector<double> out;
    double A = 1.0, Bp = 1.0;
    for (double B : prices) {
        A = A / impliedPrice(Bp, B);
        out.push_back(A);
        Bp = B;
    }
    return out;
}

}  // namespace m4f::ch10
