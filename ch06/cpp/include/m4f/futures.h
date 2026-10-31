// Section 6.2, futures: pricing in the binomial model, marking to market, and
// hedging with futures.
#ifndef M4F_CH06_FUTURES_H_
#define M4F_CH06_FUTURES_H_

#include <cmath>
#include <cstddef>
#include <vector>

namespace m4f::ch06 {

using Vec = std::vector<double>;

// Section 6.2.1: pricing. In the binomial tree (s0, u, d, r per step) the
// futures price for delivery at N is f(n) = E*(S(N) | S(n)): f(N) = S(N) and
// backwards f(n, k) = p* f(n+1, k+1) + (1 - p*) f(n+1, k). Returns f[n][k]
// after k up moves out of n.
inline std::vector<Vec> FuturesTree(double s0, double u, double d, double r,
                                    int num_steps) {
  const double q = (r - d) / (u - d);
  std::vector<Vec> f(num_steps + 1);
  for (int k = 0; k <= num_steps; ++k) {
    f[num_steps].push_back(s0 * std::pow(1 + u, k) *
                           std::pow(1 + d, num_steps - k));
  }
  for (int n = num_steps - 1; n >= 0; --n) {
    for (int k = 0; k <= n; ++k) {
      f[n].push_back(q * f[n + 1][k + 1] + (1 - q) * f[n + 1][k]);
    }
  }
  return f;
}

struct MarginDay {
  double balance;  // After settlement, before any call.
  double call;     // Margin call that restores the initial margin.
};

// Marking to market for one long contract on `size` units: each day the
// account gains size * (f(n) - f(n-1)); when it falls below the maintenance
// margin, a call restores it to the initial margin.
inline std::vector<MarginDay> MarginAccount(const Vec& prices, double size,
                                            double initial,
                                            double maintenance) {
  double balance = initial;
  std::vector<MarginDay> out;
  for (std::size_t n = 1; n < prices.size(); ++n) {
    balance += size * (prices[n] - prices[n - 1]);
    const double call = balance < maintenance ? initial - balance : 0.0;
    out.push_back({balance, call});
    balance += call;
  }
  return out;
}

// Section 6.2.2: hedging with futures. Selling h futures per unit of spot
// exposure turns a spot change dS into dS - h df.
// h* = Cov(dS, df) / Var(df).
inline double MinVarianceHedgeRatio(const Vec& ds, const Vec& df,
                                    const Vec& p) {
  double mean_s = 0.0;
  double mean_f = 0.0;
  for (std::size_t i = 0; i < p.size(); ++i) {
    mean_s += p[i] * ds[i];
    mean_f += p[i] * df[i];
  }
  double cov = 0.0;
  double var = 0.0;
  for (std::size_t i = 0; i < p.size(); ++i) {
    cov += p[i] * (ds[i] - mean_s) * (df[i] - mean_f);
    var += p[i] * (df[i] - mean_f) * (df[i] - mean_f);
  }
  return cov / var;
}

// Standard deviation of dS - h df.
inline double HedgedStd(double h, const Vec& ds, const Vec& df, const Vec& p) {
  Vec x;
  for (std::size_t i = 0; i < ds.size(); ++i) x.push_back(ds[i] - h * df[i]);
  double m = 0.0;
  for (std::size_t i = 0; i < x.size(); ++i) m += p[i] * x[i];
  double v = 0.0;
  for (std::size_t i = 0; i < x.size(); ++i)
    v += p[i] * (x[i] - m) * (x[i] - m);
  return std::sqrt(v);
}

}  // namespace m4f::ch06

#endif  // M4F_CH06_FUTURES_H_
