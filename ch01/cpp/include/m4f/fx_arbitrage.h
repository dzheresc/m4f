// Exercise 1.3: spotting a risk-free profit among currency dealers' quotes.
#ifndef M4F_CH01_FX_ARBITRAGE_H_
#define M4F_CH01_FX_ARBITRAGE_H_

#include <optional>
#include <string>
#include <vector>

namespace m4f::ch01 {

// A quote for `base` in `quote` currency: the dealer buys 1 base for `buy`
// units of quote and sells 1 base for `sell` units of quote (buy < sell).
struct FxQuote {
  std::string dealer;
  std::string base;
  std::string quote;
  double buy = 0.0;
  double sell = 0.0;
};

// Converting 1 unit of `from` into `rate` units of `to` with `dealer`.
struct FxLeg {
  std::string from;
  std::string to;
  std::string dealer;
  double rate = 0.0;
};

struct FxCycle {
  std::vector<FxLeg> legs;
  double growth = 1.0;  // Units of start currency received per unit invested.
};

class FxMarket {
 public:
  explicit FxMarket(const std::vector<FxQuote>& quotes);

  // Best direct conversion from -> to (first dealer wins ties), or nullopt if
  // no dealer quotes that pair.
  std::optional<FxLeg> BestLeg(const std::string& from,
                               const std::string& to) const;

  // Most profitable round trip start -> ... -> start through distinct
  // currencies (exhaustive search; fine for a handful of currencies).
  FxCycle BestCycle(const std::string& start) const;

 private:
  // Depth-first search from `at`, extending `path` and updating `best`.
  void Search(const std::string& start, const std::string& at, double growth,
              std::vector<std::string>& visited, std::vector<FxLeg>& path,
              FxCycle& best) const;

  std::vector<FxLeg> legs_;
  std::vector<std::string> currencies_;  // Sorted, no duplicates.
};

}  // namespace m4f::ch01

#endif  // M4F_CH01_FX_ARBITRAGE_H_
