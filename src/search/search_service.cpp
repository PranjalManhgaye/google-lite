#include "search/search_service.h"

#include <algorithm>
#include <sstream>

#include "core/normalizer.h"
#include "core/tokenizer.h"

namespace glite {

SearchService::SearchService(const InvertedIndex* index) : index_(index), cache_(256) {
  auto vocab = index_->Vocabulary();
  for (const auto& word : vocab) {
    trie_.Insert(word);
  }
  spell_.BuildVocabulary(vocab, index_->CollectionTermFreqMap());
}

std::string SearchService::BuildCacheKey(const std::string& query, const SearchOptions& options) const {
  return query + "|" + std::to_string(options.top_k) + "|" +
         std::to_string(static_cast<int>(options.field_filter)) + "|" +
         std::to_string(static_cast<int>(options.sort_mode)) + "|" + (options.explain ? "1" : "0");
}

std::string SearchService::Highlight(const std::string& text, const std::vector<std::string>& terms) const {
  std::string out = text;
  for (const auto& t : terms) {
    if (t.empty()) continue;
    std::string marker = "[" + t + "]";
    std::size_t pos = 0;
    while ((pos = out.find(t, pos)) != std::string::npos) {
      out.replace(pos, t.size(), marker);
      pos += marker.size();
    }
  }
  return out;
}

std::string SearchService::BuildSnippet(const Document& doc, const std::vector<std::string>& terms) const {
  std::string body = doc.body;
  std::size_t best_pos = std::string::npos;
  std::size_t best_term_len = 0;
  for (const auto& t : terms) {
    auto p = body.find(t);
    if (p != std::string::npos && (best_pos == std::string::npos || p < best_pos)) {
      best_pos = p;
      best_term_len = t.size();
    }
  }
  if (best_pos == std::string::npos) {
    std::string chunk = body.substr(0, std::min<std::size_t>(140, body.size()));
    return Highlight(chunk, terms);
  }
  std::size_t start = best_pos > 45 ? best_pos - 45 : 0;
  std::size_t end = std::min(body.size(), best_pos + best_term_len + 95);
  std::string chunk = body.substr(start, end - start);
  return (start > 0 ? "... " : "") + Highlight(chunk, terms) + (end < body.size() ? " ..." : "");
}

std::vector<SearchResult> SearchService::Search(const std::string& query, const SearchOptions& options,
                                                bool* cache_hit) {
  SearchOptions effective = options;
  std::string normalized_query = query;
  if (normalized_query.rfind("title:", 0) == 0) {
    effective.field_filter = SearchFieldFilter::kTitle;
    normalized_query = normalized_query.substr(6);
  } else if (normalized_query.rfind("body:", 0) == 0) {
    effective.field_filter = SearchFieldFilter::kBody;
    normalized_query = normalized_query.substr(5);
  }
  if (normalized_query.rfind("sort:title ", 0) == 0) {
    effective.sort_mode = SortMode::kTitle;
    normalized_query = normalized_query.substr(11);
  } else if (normalized_query.rfind("sort:length ", 0) == 0) {
    effective.sort_mode = SortMode::kLength;
    normalized_query = normalized_query.substr(12);
  }

  const auto key = BuildCacheKey(normalized_query, effective);
  std::vector<SearchResult> cached;
  if (cache_.Get(key, &cached)) {
    if (cache_hit) *cache_hit = true;
    return cached;
  }
  if (cache_hit) *cache_hit = false;

  auto exec_result = executor_.Search(index_, normalized_query, effective);
  std::vector<SearchResult> out = exec_result.results;
  const auto terms = exec_result.scoring_terms;
  for (auto& r : out) {
    r.title = Highlight(r.title, terms);
    r.snippet = Highlight(r.snippet, terms);
  }
  cache_.Put(key, out);
  return out;
}

std::vector<std::string> SearchService::LiveSuggestions(const std::string& query_prefix) const {
  std::vector<std::string> merged;
  const auto p = trie_.Suggest(query_prefix, 5);
  merged.insert(merged.end(), p.begin(), p.end());
  const auto t = spell_.Suggest(query_prefix, {}, 2, 5);
  for (const auto& s : t) {
    if (std::find(merged.begin(), merged.end(), s) == merged.end()) merged.push_back(s);
  }
  return merged;
}

std::vector<std::string> SearchService::TypoSuggestions(const std::string& term) const {
  return spell_.Suggest(term, {}, 2, 8);
}

}  // namespace glite
