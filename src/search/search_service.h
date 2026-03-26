#pragma once

#include <string>
#include <vector>

#include "index/inverted_index.h"
#include "query/query_executor.h"
#include "search/query_cache.h"
#include "search/search_types.h"
#include "suggest/spell_corrector.h"
#include "suggest/trie.h"

namespace glite {

class SearchService {
 public:
  explicit SearchService(const InvertedIndex* index);
  std::vector<SearchResult> Search(const std::string& query, const SearchOptions& options,
                                   bool* cache_hit = nullptr);
  std::vector<std::string> LiveSuggestions(const std::string& query_prefix) const;
  std::vector<std::string> TypoSuggestions(const std::string& term) const;

 private:
  std::string BuildCacheKey(const std::string& query, const SearchOptions& options) const;
  std::string BuildSnippet(const Document& doc, const std::vector<std::string>& terms) const;
  std::string Highlight(const std::string& text, const std::vector<std::string>& terms) const;

  const InvertedIndex* index_;
  QueryExecutor executor_;
  Trie trie_;
  SpellCorrector spell_;
  mutable QueryCache cache_;
};

}  // namespace glite
