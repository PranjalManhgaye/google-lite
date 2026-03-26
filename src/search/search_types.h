#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace glite {

enum class SearchFieldFilter { kAll, kTitle, kBody };
enum class SortMode { kRelevance, kTitle, kLength };

struct SearchOptions {
  std::size_t top_k{10};
  SearchFieldFilter field_filter{SearchFieldFilter::kAll};
  SortMode sort_mode{SortMode::kRelevance};
  bool explain{false};
};

struct SearchResult {
  std::uint32_t doc_id{};
  double score{};
  std::string title;
  std::string snippet;
  std::unordered_map<std::string, double> explain_terms;
};

}  // namespace glite
