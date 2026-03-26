#include "crawl/html_extractor.h"

#include <regex>

namespace glite {

ExtractedPage HtmlExtractor::Extract(const std::string& html, const std::string& base_url) const {
  (void)base_url;
  ExtractedPage out;
  std::smatch m;
  if (std::regex_search(html, m, std::regex("<title[^>]*>(.*?)</title>", std::regex::icase))) {
    out.title = m[1].str();
  }
  std::string body = std::regex_replace(html, std::regex("<script[\\s\\S]*?</script>", std::regex::icase), " ");
  body = std::regex_replace(body, std::regex("<style[\\s\\S]*?</style>", std::regex::icase), " ");
  body = std::regex_replace(body, std::regex("<[^>]+>"), " ");
  body = std::regex_replace(body, std::regex("\\s+"), " ");
  out.text = body;

  const std::regex link_re("<a[^>]*href=[\"']([^\"']+)[\"'][^>]*>", std::regex::icase);
  auto begin = std::sregex_iterator(html.begin(), html.end(), link_re);
  auto end = std::sregex_iterator();
  for (auto it = begin; it != end; ++it) {
    std::string href = (*it)[1].str();
    if (href.rfind("http://", 0) == 0 || href.rfind("https://", 0) == 0) out.links.push_back(href);
  }
  return out;
}

}  // namespace glite
