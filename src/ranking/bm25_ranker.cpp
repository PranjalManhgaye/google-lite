#include "ranking/bm25_ranker.h"

#include <cmath>
#include <unordered_set>

namespace glite {

BM25ScoreBundle BM25Ranker::Score(
    const InvertedIndex& index,
    const std::vector<std::string>& terms,
    const std::vector<std::uint32_t>& candidate_docs,
    bool collect_explain) const {
  BM25ScoreBundle bundle;
  if (index.DocCount() == 0) {
    return bundle;
  }

  std::unordered_set<std::uint32_t> candidate_set(candidate_docs.begin(), candidate_docs.end());
  for (const auto& term : terms) {
    const auto* postings = index.GetPostings(term);
    if (postings == nullptr || postings->empty()) {
      continue;
    }
    const double df = static_cast<double>(index.DocFreq(term));
    const double n = static_cast<double>(index.DocCount());
    const double idf = std::log(1.0 + (n - df + 0.5) / (df + 0.5));

    for (const auto& p : *postings) {
      if (candidate_set.find(p.doc_id) == candidate_set.end()) {
        continue;
      }
      const double tf = static_cast<double>(p.positions.size());
      const double dl = static_cast<double>(index.DocLength(p.doc_id));
      const double avg = index.AvgDocLength() > 0.0 ? index.AvgDocLength() : 1.0;
      const double norm = k1_ * (1.0 - b_ + b_ * (dl / avg));
      const double s = idf * ((tf * (k1_ + 1.0)) / (tf + norm));
      bundle.scores[p.doc_id] += s;
      if (collect_explain) {
        bundle.term_contributions[p.doc_id][term] += s;
      }
    }
  }
  return bundle;
}

}  // namespace glite
