#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/document.h"

namespace glite {

struct CrawlOptions {
  std::size_t max_pages{50};
  std::size_t max_depth{1};
  bool same_domain_only{true};
};

class Crawler {
 public:
  std::vector<Document> Crawl(const std::vector<std::string>& seed_urls, const CrawlOptions& options) const;

 private:
  std::string FetchUrl(const std::string& url) const;
  std::string DomainOf(const std::string& url) const;
};

}  // namespace glite
