#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "index/inverted_index.h"

namespace glite {

struct BM25ScoreBundle {
  std::unordered_map<std::uint32_t, double> scores;
  std::unordered_map<std::uint32_t, std::unordered_map<std::string, double>> term_contributions;
};

class BM25Ranker {
 public:
  BM25Ranker(double k1 = 1.2, double b = 0.75) : k1_(k1), b_(b) {}
  BM25ScoreBundle Score(
      const InvertedIndex& index,
      const std::vector<std::string>& terms,
      const std::vector<std::uint32_t>& candidate_docs,
      bool collect_explain = false) const;

 private:
  double k1_;
  double b_;
};

}  // namespace glite
