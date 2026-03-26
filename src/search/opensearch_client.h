#pragma once

#include <string>
#include <vector>

#include "core/document.h"

namespace glite {

struct OpenSearchHit {
  std::string id;
  std::string url;
  std::string title;
  std::string snippet;
  double score{0.0};
};

class OpenSearchClient {
 public:
  OpenSearchClient(std::string base_url, std::string index_name)
      : base_url_(std::move(base_url)), index_name_(std::move(index_name)) {}
  bool EnsureIndex();
  bool IsAvailable() const;
  bool UpsertByUrl(const Document& doc);
  bool GetByUrl(const std::string& url, OpenSearchHit* hit);
  bool GetById(const std::string& id, OpenSearchHit* hit);
  std::vector<OpenSearchHit> Search(const std::string& query, std::size_t top_k = 10);
  std::vector<std::string> Suggest(const std::string& query, std::size_t top_k = 6);

 private:
  std::string JsonEscape(const std::string& s) const;
  std::string Curl(const std::string& method, const std::string& path, const std::string& body = "") const;
  std::string UrlKey(const std::string& url) const;
  std::string ExtractJsonField(const std::string& json, const std::string& key) const;
  double ExtractScore(const std::string& json) const;
  std::string base_url_;
  std::string index_name_;
};

}  // namespace glite
