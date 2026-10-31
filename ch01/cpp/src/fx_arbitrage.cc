#include "m4f/fx_arbitrage.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace m4f::ch01 {

FxMarket::FxMarket(const std::vector<FxQuote>& quotes) {
  for (const FxQuote& q : quotes) {
    // Sell base to the dealer, or buy base from the dealer.
    legs_.push_back({q.base, q.quote, q.dealer, q.buy});
    legs_.push_back({q.quote, q.base, q.dealer, 1.0 / q.sell});
    currencies_.push_back(q.base);
    currencies_.push_back(q.quote);
  }
  std::sort(currencies_.begin(), currencies_.end());
  currencies_.erase(std::unique(currencies_.begin(), currencies_.end()),
                    currencies_.end());

  // Best leg per (from, to); legs are scanned in order and only a strictly
  // better rate replaces the current best, so the first dealer wins ties.
  const std::size_t n = currencies_.size();
  best_leg_.assign(n * n, kNoLeg);
  for (std::size_t k = 0; k < legs_.size(); ++k) {
    const std::size_t i =
        static_cast<std::size_t>(CurrencyIndex(legs_[k].from));
    const std::size_t j = static_cast<std::size_t>(CurrencyIndex(legs_[k].to));
    int& best = best_leg_[i * n + j];
    if (best == kNoLeg ||
        legs_[k].rate > legs_[static_cast<std::size_t>(best)].rate) {
      best = static_cast<int>(k);
    }
  }
}

int FxMarket::CurrencyIndex(const std::string& currency) const {
  const auto it =
      std::lower_bound(currencies_.begin(), currencies_.end(), currency);
  if (it == currencies_.end() || *it != currency) return -1;
  return static_cast<int>(it - currencies_.begin());
}

std::optional<FxLeg> FxMarket::BestLeg(const std::string& from,
                                       const std::string& to) const {
  const int i = CurrencyIndex(from);
  const int j = CurrencyIndex(to);
  if (i < 0 || j < 0) return std::nullopt;
  const int k = BestLegIndex(i, j);
  if (k == kNoLeg) return std::nullopt;
  return legs_[static_cast<std::size_t>(k)];
}

FxCycle FxMarket::BestCycle(const std::string& start) const {
  FxCycle best{{}, 0.0};
  const int s = CurrencyIndex(start);
  if (s < 0) return best;
  SearchState state{s, std::vector<char>(currencies_.size(), 0), {}, {}, 0.0};
  state.visited[static_cast<std::size_t>(s)] = 1;
  Search(s, 1.0, state);
  best.growth = state.best_growth;
  for (int k : state.best_path) {
    best.legs.push_back(legs_[static_cast<std::size_t>(k)]);
  }
  return best;
}

void FxMarket::Search(int at, double growth, SearchState& state) const {
  const int n = static_cast<int>(currencies_.size());
  for (int next = 0; next < n; ++next) {
    const int k = BestLegIndex(at, next);
    if (k == kNoLeg) continue;
    const bool closes = next == state.start;
    if (!closes && state.visited[static_cast<std::size_t>(next)]) continue;
    if (closes && state.path.empty()) continue;
    state.path.push_back(k);
    const double new_growth = growth * legs_[static_cast<std::size_t>(k)].rate;
    if (closes) {
      if (new_growth > state.best_growth) {
        state.best_growth = new_growth;
        state.best_path = state.path;
      }
    } else {
      state.visited[static_cast<std::size_t>(next)] = 1;
      Search(next, new_growth, state);
      state.visited[static_cast<std::size_t>(next)] = 0;
    }
    state.path.pop_back();
  }
}

}  // namespace m4f::ch01
