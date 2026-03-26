#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace glite {

class SpellCorrector {
 public:
  void BuildVocabulary(const std::vector<std::string>& vocab,
                       const std::unordered_map<std::string, std::uint32_t>& term_freq = {});
  std::vector<std::string> Suggest(const std::string& term,
                                   const std::vector<std::string>& query_context = {},
                                   std::size_t max_edits = 2,
                                   std::size_t max_results = 5) const;

 private:
  int EditDistanceBounded(const std::string& a, const std::string& b, int max_edits) const;
  std::vector<std::string> vocab_;
  std::unordered_map<std::string, std::uint32_t> term_freq_;
};

}  // namespace glite
