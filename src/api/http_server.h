#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "index/index_snapshot.h"
#include "io/doc_store.h"
#include "search/opensearch_client.h"
#include "search/search_service.h"

namespace glite {

class HttpServer {
 public:
  HttpServer(SearchService* service, InvertedIndex* index, IndexSnapshot* snapshot, DocStore* store,
             OpenSearchClient* os_client)
      : service_(service), index_(index), snapshot_(snapshot), store_(store), os_client_(os_client), request_count_(0) {}
  bool Start(std::uint16_t port);

 private:
  std::string HandleRequest(const std::string& req);
  std::string JsonEscape(const std::string& s) const;
  std::string HandleSearchJson(const std::string& body);
  std::string HandleSearchHtml(const std::string& path);
  std::string HandleSuggest(const std::string& path);
  std::string HandleDoc(const std::string& path);
  std::string HandleHealth();
  std::string HandleOpenSearchXml();
  std::string QueryParam(const std::string& path, const std::string& key) const;
  std::string UrlDecode(const std::string& s) const;
  bool IsUrlQuery(const std::string& q) const;
  std::string HandleUrlPreview(const std::string& url);
  bool RebuildIndexFromStore();
  std::string ReadFileText(const std::string& path) const;
  std::string Response(const std::string& body, const std::string& content_type = "application/json",
                       const std::string& status = "200 OK");

  SearchService* service_;
  InvertedIndex* index_;
  IndexSnapshot* snapshot_;
  DocStore* store_;
  OpenSearchClient* os_client_;
  std::atomic<std::uint64_t> request_count_;
};

}  // namespace glite
