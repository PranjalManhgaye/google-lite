#pragma once

#include <string>
#include <vector>

#include "core/document.h"

namespace glite {

class WikiLoader {
 public:
  // Expected format: id<TAB>title<TAB>body
  std::vector<Document> LoadTsv(const std::string& path, std::size_t max_docs = 0) const;
};

}  // namespace glite
