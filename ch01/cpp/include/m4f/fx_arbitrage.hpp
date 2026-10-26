// Exercise 1.3: spotting a risk-free profit among currency dealers' quotes.
// A quote for `base` in `quote` currency: the dealer buys 1 base for `buy`
// units of quote and sells 1 base for `sell` units of quote (buy < sell).
#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace m4f::ch01 {

struct FxQuote {
    std::string dealer, base, quote;
    double buy, sell;
};

// Converting 1 unit of `from` into `rate` units of `to` with `dealer`.
struct FxLeg {
    std::string from, to, dealer;
    double rate;
};

struct FxCycle {
    std::vector<FxLeg> legs;
    double growth = 1.0;  // units of start currency received per unit invested
};

class FxMarket {
public:
    explicit FxMarket(const std::vector<FxQuote>& quotes) {
        for (const auto& q : quotes) {
            legs_.push_back({q.base, q.quote, q.dealer, q.buy});        // sell base to dealer
            legs_.push_back({q.quote, q.base, q.dealer, 1.0 / q.sell});  // buy base from dealer
            for (const auto& c : {q.base, q.quote})
                if (std::find(ccys_.begin(), ccys_.end(), c) == ccys_.end()) ccys_.push_back(c);
        }
        std::sort(ccys_.begin(), ccys_.end());
    }

    // Best direct conversion from -> to (first dealer wins ties).
    std::optional<FxLeg> bestLeg(const std::string& from, const std::string& to) const {
        std::optional<FxLeg> best;
        for (const auto& l : legs_)
            if (l.from == from && l.to == to && (!best || l.rate > best->rate)) best = l;
        return best;
    }

    // Most profitable round trip start -> ... -> start through distinct
    // currencies (exhaustive search; fine for a handful of currencies).
    FxCycle bestCycle(const std::string& start) const {
        FxCycle best{{}, 0.0};
        std::vector<FxLeg> path;
        std::vector<std::string> visited{start};
        search(start, start, 1.0, visited, path, best);
        return best;
    }

private:
    void search(const std::string& start, const std::string& at, double growth,
                std::vector<std::string>& visited, std::vector<FxLeg>& path,
                FxCycle& best) const {
        for (const auto& next : ccys_) {
            const auto leg = bestLeg(at, next);
            if (!leg) continue;
            const bool closes = next == start;
            if (!closes && std::find(visited.begin(), visited.end(), next) != visited.end())
                continue;
            if (closes && path.empty()) continue;
            path.push_back(*leg);
            const double g = growth * leg->rate;
            if (closes) {
                if (g > best.growth) best = {path, g};
            } else {
                visited.push_back(next);
                search(start, next, g, visited, path, best);
                visited.pop_back();
            }
            path.pop_back();
        }
    }

    std::vector<FxLeg> legs_;
    std::vector<std::string> ccys_;
};

}  // namespace m4f::ch01
