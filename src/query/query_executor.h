#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "index/inverted_index.h"
#include "query/query_parser.h"
#include "ranking/bm25_ranker.h"
#include "search/search_types.h"

namespace glite {

struct SearchResponse {
  std::vector<SearchResult> results;
  std::vector<std::string> scoring_terms;
};

class QueryExecutor {
 public:
  QueryExecutor() = default;
  SearchResponse Search(const InvertedIndex* index, const std::string& query,
                        const SearchOptions& options) const;

 private:
  std::vector<std::uint32_t> EvalTerm(const InvertedIndex& index, const std::string& term) const;
  std::vector<std::uint32_t> EvalPhrase(const InvertedIndex& index, const std::string& phrase) const;
  std::vector<std::uint32_t> SetAnd(const std::vector<std::uint32_t>& a,
                                    const std::vector<std::uint32_t>& b) const;
  std::vector<std::uint32_t> SetOr(const std::vector<std::uint32_t>& a,
                                   const std::vector<std::uint32_t>& b) const;
  std::vector<std::uint32_t> SetNot(const std::vector<std::uint32_t>& universe,
                                    const std::vector<std::uint32_t>& b) const;
  std::vector<std::string> ExtractScoringTerms(const std::vector<QueryToken>& rpn) const;
  std::vector<std::uint32_t> ApplyFieldFilter(const InvertedIndex* index,
                                              const std::vector<std::uint32_t>& candidates,
                                              SearchFieldFilter filter) const;
  std::string Highlight(const std::string& text, const std::vector<std::string>& terms) const;
  std::string BuildSnippet(const InvertedIndex* index, std::uint32_t doc_id,
                           const std::vector<std::string>& terms) const;

  QueryParser parser_;
  BM25Ranker ranker_;
};

}  // namespace glite
