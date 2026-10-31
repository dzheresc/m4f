// Section 3.1, dynamics of stock prices: returns, logarithmic returns and their
// expectations over a finite set of scenarios.
//
// Book notation: S(0), S(1) are `s0`, `s1`; K is a one-step return and k a
// logarithmic return.
#ifndef M4F_CH03_RETURNS_H_
#define M4F_CH03_RETURNS_H_

#include <cmath>
#include <cstddef>
#include <vector>

namespace m4f::ch03 {

// 3.1.1 Return: K(1) = (S(1) + div - S(0)) / S(0).
inline double OneStepReturn(double s0, double s1, double dividend = 0.0) {
  return (s1 + dividend - s0) / s0;
}

// Logarithmic return k(1) = ln(S(1) / S(0)).
inline double LogReturn(double s0, double s1) { return std::log(s1 / s0); }

// One-step returns K(n) along a price path S(0), S(1), ...
inline std::vector<double> Returns(const std::vector<double>& path) {
  std::vector<double> out;
  for (std::size_t i = 1; i < path.size(); ++i) {
    out.push_back(OneStepReturn(path[i - 1], path[i]));
  }
  return out;
}

// One-step logarithmic returns k(n) along a price path.
inline std::vector<double> LogReturns(const std::vector<double>& path) {
  std::vector<double> out;
  for (std::size_t i = 1; i < path.size(); ++i) {
    out.push_back(LogReturn(path[i - 1], path[i]));
  }
  return out;
}

// K(0, n) from one-step returns: 1 + K(0, n) = (1 + K(1)) ... (1 + K(n)).
inline double CompoundReturn(const std::vector<double>& returns) {
  double growth = 1.0;
  for (double k : returns) growth *= 1 + k;
  return growth - 1;
}

// 3.1.2 Expected return: E(X) over a finite set of scenarios.
inline double Expectation(const std::vector<double>& values,
                          const std::vector<double>& probs) {
  double sum = 0.0;
  for (std::size_t i = 0; i < values.size(); ++i) sum += values[i] * probs[i];
  return sum;
}

inline double Variance(const std::vector<double>& values,
                       const std::vector<double>& probs) {
  const double mean = Expectation(values, probs);
  double sum = 0.0;
  for (std::size_t i = 0; i < values.size(); ++i) {
    const double deviation = values[i] - mean;
    sum += probs[i] * deviation * deviation;
  }
  return sum;
}

}  // namespace m4f::ch03

#endif  // M4F_CH03_RETURNS_H_
