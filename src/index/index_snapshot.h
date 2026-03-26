#pragma once

#include <string>

#include "index/inverted_index.h"
#include "io/doc_store.h"

namespace glite {

class IndexSnapshot {
 public:
  explicit IndexSnapshot(std::string dir) : dir_(std::move(dir)), store_(dir_) {}
  bool SaveFromIndex(const InvertedIndex& index) const;
  InvertedIndex LoadToIndex(bool enable_compression = true) const;
  InvertedIndex IncrementalRebuild(const std::vector<Document>& new_docs, bool enable_compression = true) const;

 private:
  std::string dir_;
  DocStore store_;
};

}  // namespace glite
