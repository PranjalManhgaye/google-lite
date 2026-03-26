#pragma once

#include <string>
#include <vector>

namespace glite {

enum class QueryTokenType { kTerm, kPhrase, kAnd, kOr, kNot, kLParen, kRParen };

struct QueryToken {
  QueryTokenType type;
  std::string value;
};

class QueryParser {
 public:
  std::vector<QueryToken> ParseToRpn(const std::string& query) const;
};

}  // namespace glite
