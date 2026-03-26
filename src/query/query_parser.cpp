#include "query/query_parser.h"

#include <algorithm>
#include <cctype>
#include <stack>

namespace glite {
namespace {

int Precedence(QueryTokenType t) {
  if (t == QueryTokenType::kNot) return 3;
  if (t == QueryTokenType::kAnd) return 2;
  if (t == QueryTokenType::kOr) return 1;
  return 0;
}

bool IsOperator(QueryTokenType t) {
  return t == QueryTokenType::kAnd || t == QueryTokenType::kOr || t == QueryTokenType::kNot;
}

std::string ToUpper(const std::string& s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::toupper(c));
  });
  return out;
}

}  // namespace

std::vector<QueryToken> QueryParser::ParseToRpn(const std::string& query) const {
  std::vector<QueryToken> infix;
  std::string cur;
  for (std::size_t i = 0; i < query.size(); ++i) {
    char ch = query[i];
    if (ch == '"') {
      if (!cur.empty()) {
        const auto up = ToUpper(cur);
        if (up == "AND") infix.push_back({QueryTokenType::kAnd, ""});
        else if (up == "OR") infix.push_back({QueryTokenType::kOr, ""});
        else if (up == "NOT") infix.push_back({QueryTokenType::kNot, ""});
        else infix.push_back({QueryTokenType::kTerm, cur});
        cur.clear();
      }
      std::string phrase;
      ++i;
      while (i < query.size() && query[i] != '"') {
        phrase.push_back(query[i++]);
      }
      if (!phrase.empty()) {
        infix.push_back({QueryTokenType::kPhrase, phrase});
      }
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      if (!cur.empty()) {
        const auto up = ToUpper(cur);
        if (up == "AND") infix.push_back({QueryTokenType::kAnd, ""});
        else if (up == "OR") infix.push_back({QueryTokenType::kOr, ""});
        else if (up == "NOT") infix.push_back({QueryTokenType::kNot, ""});
        else infix.push_back({QueryTokenType::kTerm, cur});
        cur.clear();
      }
      continue;
    }
    if (ch == '(' || ch == ')') {
      if (!cur.empty()) {
        const auto up = ToUpper(cur);
        if (up == "AND") infix.push_back({QueryTokenType::kAnd, ""});
        else if (up == "OR") infix.push_back({QueryTokenType::kOr, ""});
        else if (up == "NOT") infix.push_back({QueryTokenType::kNot, ""});
        else infix.push_back({QueryTokenType::kTerm, cur});
        cur.clear();
      }
      infix.push_back({ch == '(' ? QueryTokenType::kLParen : QueryTokenType::kRParen, ""});
      continue;
    }
    cur.push_back(ch);
  }
  if (!cur.empty()) {
    const auto up = ToUpper(cur);
    if (up == "AND") infix.push_back({QueryTokenType::kAnd, ""});
    else if (up == "OR") infix.push_back({QueryTokenType::kOr, ""});
    else if (up == "NOT") infix.push_back({QueryTokenType::kNot, ""});
    else infix.push_back({QueryTokenType::kTerm, cur});
  }

  std::vector<QueryToken> output;
  std::stack<QueryToken> ops;
  for (const auto& tok : infix) {
    if (tok.type == QueryTokenType::kTerm || tok.type == QueryTokenType::kPhrase) {
      output.push_back(tok);
    } else if (IsOperator(tok.type)) {
      while (!ops.empty() && IsOperator(ops.top().type) &&
             Precedence(ops.top().type) >= Precedence(tok.type)) {
        output.push_back(ops.top());
        ops.pop();
      }
      ops.push(tok);
    } else if (tok.type == QueryTokenType::kLParen) {
      ops.push(tok);
    } else if (tok.type == QueryTokenType::kRParen) {
      while (!ops.empty() && ops.top().type != QueryTokenType::kLParen) {
        output.push_back(ops.top());
        ops.pop();
      }
      if (!ops.empty() && ops.top().type == QueryTokenType::kLParen) {
        ops.pop();
      }
    }
  }
  while (!ops.empty()) {
    output.push_back(ops.top());
    ops.pop();
  }
  return output;
}

}  // namespace glite
