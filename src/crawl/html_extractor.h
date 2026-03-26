#pragma once

#include <string>
#include <vector>

namespace glite {

struct ExtractedPage {
  std::string title;
  std::string text;
  std::vector<std::string> links;
};

class HtmlExtractor {
 public:
  ExtractedPage Extract(const std::string& html, const std::string& base_url) const;
};

}  // namespace glite
