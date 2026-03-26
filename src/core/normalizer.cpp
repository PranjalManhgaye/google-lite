#include "core/normalizer.h"

#include <algorithm>
#include <cctype>

namespace glite {

Normalizer::Normalizer()
    : stopwords_({"a",   "an",  "and", "are", "as",  "at", "be",   "by",
                  "for", "from","in",  "is",  "it",  "of", "on",   "or",
                  "that","the", "to",  "was", "were","with"}) {}

std::string Normalizer::NormalizeToken(const std::string& token) const {
  std::string normalized;
  normalized.reserve(token.size());
  for (char ch : token) {
    unsigned char uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch)) {
      normalized.push_back(static_cast<char>(std::tolower(uch)));
    }
  }
  return normalized;
}

bool Normalizer::IsStopword(const std::string& token) const {
  return stopwords_.find(token) != stopwords_.end();
}

}  // namespace glite
