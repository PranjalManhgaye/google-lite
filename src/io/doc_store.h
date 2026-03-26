#pragma once

#include <string>
#include <vector>

#include "core/document.h"

namespace glite {

class DocStore {
 public:
  explicit DocStore(std::string root_dir) : root_dir_(std::move(root_dir)) {}
  bool SaveDocuments(const std::vector<Document>& docs) const;
  std::vector<Document> LoadDocuments() const;
  bool SaveFrontier(const std::vector<std::string>& urls) const;
  std::vector<std::string> LoadFrontier() const;
  const std::string& RootDir() const { return root_dir_; }

 private:
  std::string DocsPath() const;
  std::string FrontierPath() const;
  std::string root_dir_;
};

}  // namespace glite
