#include "core/tokenizer.h"

#include <cctype>

namespace glite {

std::vector<std::string> Tokenizer::Split(const std::string& text) const {
  std::vector<std::string> out;
  std::string cur;
  for (char ch : text) {
    if (std::isalnum(static_cast<unsigned char>(ch))) {
      cur.push_back(ch);
    } else if (!cur.empty()) {
      out.push_back(cur);
      cur.clear();
    }
  }
  if (!cur.empty()) {
    out.push_back(cur);
  }
  return out;
}

}  // namespace glite
