#pragma once

#include <string>
#include <vector>

namespace glite {

class Tokenizer {
 public:
  std::vector<std::string> Split(const std::string& text) const;
};

}  // namespace glite
