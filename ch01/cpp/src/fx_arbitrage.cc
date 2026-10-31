#include "m4f/fx_arbitrage.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace m4f::ch01 {

FxMarket::FxMarket(const std::vector<FxQuote>& quotes) {
  for (const FxQuote& q : quotes) {
    // Sell base to the dealer, or buy base from the dealer.
    legs_.push_back({q.base, q.quote, q.dealer, q.buy});
    legs_.push_back({q.quote, q.base, q.dealer, 1.0 / q.sell});
    for (const std::string& currency : {q.base, q.quote}) {
      if (std::find(currencies_.begin(), currencies_.end(), currency) ==
          currencies_.end()) {
        currencies_.push_back(currency);
      }
    }
  }
  std::sort(currencies_.begin(), currencies_.end());
}

std::optional<FxLeg> FxMarket::BestLeg(const std::string& from,
                                       const std::string& to) const {
  std::optional<FxLeg> best;
  for (const FxLeg& leg : legs_) {
    if (leg.from == from && leg.to == to && (!best || leg.rate > best->rate)) {
      best = leg;
    }
  }
  return best;
}

FxCycle FxMarket::BestCycle(const std::string& start) const {
  FxCycle best{{}, 0.0};
  std::vector<FxLeg> path;
  std::vector<std::string> visited{start};
  Search(start, start, 1.0, visited, path, best);
  return best;
}

void FxMarket::Search(const std::string& start, const std::string& at,
                      double growth, std::vector<std::string>& visited,
                      std::vector<FxLeg>& path, FxCycle& best) const {
  for (const std::string& next : currencies_) {
    const std::optional<FxLeg> leg = BestLeg(at, next);
    if (!leg) continue;
    const bool closes = next == start;
    if (!closes &&
        std::find(visited.begin(), visited.end(), next) != visited.end()) {
      continue;
    }
    if (closes && path.empty()) continue;
    path.push_back(*leg);
    const double new_growth = growth * leg->rate;
    if (closes) {
      if (new_growth > best.growth) best = {path, new_growth};
    } else {
      visited.push_back(next);
      Search(start, next, new_growth, visited, path, best);
      visited.pop_back();
    }
    path.pop_back();
  }
}

}  // namespace m4f::ch01
