// Section 1.5: forward contracts. A portfolio (x, y, z) adds z forward
// contracts (z > 0 long, z < 0 short) with forward price F to x shares and
// y bonds; entering a forward costs nothing at t=0.
//
// The forward price F is called `delivery_price` below.
#ifndef M4F_CH01_FORWARD_H_
#define M4F_CH01_FORWARD_H_

#include <optional>

namespace m4f::ch01 {

struct ForwardPortfolio {
  double x = 0.0;  // Shares.
  double y = 0.0;  // Bonds.
  double z = 0.0;  // Forward contracts.
};

inline double LongForwardPayoff(double s1, double delivery_price) {
  return s1 - delivery_price;
}
inline double ShortForwardPayoff(double s1, double delivery_price) {
  return delivery_price - s1;
}

// Proposition 1.2: the no-arbitrage forward price
//   F = S(0) A(1) / A(0) = S(0) (1 + r).
inline double ForwardPrice(double s0, double a0, double a1) {
  return s0 * a1 / a0;
}

inline double Value0(const ForwardPortfolio& portfolio, double s0, double a0) {
  return portfolio.x * s0 + portfolio.y * a0;
}
inline double Value1(const ForwardPortfolio& portfolio, double s1, double a1,
                     double delivery_price) {
  return portfolio.x * s1 + portfolio.y * a1 +
         portfolio.z * (s1 - delivery_price);
}

// Cash-and-carry arbitrage against a mispriced forward, one contract:
//   F > fair: borrow S(0), buy a share, short the forward  -> V(1) = F - fair
//   F < fair: short a share, invest S(0), long the forward -> V(1) = fair - F
// V(0) = 0 and V(1) does not depend on S(1). Returns nullopt if F is fair.
inline std::optional<ForwardPortfolio> ForwardArbitrage(double s0, double a0,
                                                        double a1,
                                                        double delivery_price) {
  const double fair = ForwardPrice(s0, a0, a1);
  if (delivery_price > fair) return ForwardPortfolio{1.0, -s0 / a0, -1.0};
  if (delivery_price < fair) return ForwardPortfolio{-1.0, s0 / a0, 1.0};
  return std::nullopt;
}

// Exercise 1.6: forward on a currency. Holding the foreign currency earns the
// foreign rate r_f (a negative carrying cost), so F = S(0)(1+r)/(1+r_f) and
//   r_f = S(0)(1+r)/F - 1.
inline double ImpliedForeignRate(double s0, double delivery_price, double r) {
  return s0 * (1 + r) / delivery_price - 1;
}

}  // namespace m4f::ch01

#endif  // M4F_CH01_FORWARD_H_
