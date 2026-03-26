#pragma once

#include <string>
#include <unordered_set>

namespace glite {

class Normalizer {
 public:
  Normalizer();
  std::string NormalizeToken(const std::string& token) const;
  bool IsStopword(const std::string& token) const;

 private:
  std::unordered_set<std::string> stopwords_;
};

}  // namespace glite
