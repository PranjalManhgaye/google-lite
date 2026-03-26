#pragma once

#include <cstddef>
#include <vector>

#include "core/document.h"
#include "index/inverted_index.h"

namespace glite {

class ParallelIndexBuilder {
 public:
  static InvertedIndex Build(const std::vector<Document>& docs, std::size_t thread_count = 0,
                             bool enable_compression = false);
};

}  // namespace glite
