#include "suggest/spell_corrector.h"

#include <algorithm>
#include <limits>

namespace glite {

void SpellCorrector::BuildVocabulary(const std::vector<std::string>& vocab,
                                     const std::unordered_map<std::string, std::uint32_t>& term_freq) {
  vocab_ = vocab;
  term_freq_ = term_freq;
}

int SpellCorrector::EditDistanceBounded(const std::string& a, const std::string& b, int max_edits) const {
  if (std::abs(static_cast<int>(a.size()) - static_cast<int>(b.size())) > max_edits) {
    return max_edits + 1;
  }
  std::vector<int> prev(b.size() + 1), cur(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j) prev[j] = static_cast<int>(j);
  for (std::size_t i = 1; i <= a.size(); ++i) {
    cur[0] = static_cast<int>(i);
    int row_min = cur[0];
    for (std::size_t j = 1; j <= b.size(); ++j) {
      int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
      cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
      row_min = std::min(row_min, cur[j]);
    }
    if (row_min > max_edits) return max_edits + 1;
    prev.swap(cur);
  }
  return prev[b.size()];
}

std::vector<std::string> SpellCorrector::Suggest(const std::string& term,
                                                 const std::vector<std::string>& query_context,
                                                 std::size_t max_edits,
                                                 std::size_t max_results) const {
  struct Candidate {
    std::string word;
    int dist;
    std::uint32_t freq;
    int context_bonus;
  };
  std::unordered_map<std::string, bool> context_set;
  for (const auto& c : query_context) context_set[c] = true;
  std::vector<Candidate> candidates;
  for (const auto& w : vocab_) {
    int dist = EditDistanceBounded(term, w, static_cast<int>(max_edits));
    if (dist <= static_cast<int>(max_edits)) {
      auto it = term_freq_.find(w);
      std::uint32_t freq = (it == term_freq_.end()) ? 0U : it->second;
      int context_bonus = context_set.find(w) != context_set.end() ? 1 : 0;
      candidates.push_back({w, dist, freq, context_bonus});
    }
  }
  std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
    if (a.dist == b.dist) {
      if (a.context_bonus == b.context_bonus) {
        if (a.freq == b.freq) return a.word < b.word;
        return a.freq > b.freq;
      }
      return a.context_bonus > b.context_bonus;
    }
    return a.dist < b.dist;
  });
  std::vector<std::string> out;
  for (const auto& c : candidates) {
    out.push_back(c.word);
    if (out.size() >= max_results) break;
  }
  return out;
}

}  // namespace glite
