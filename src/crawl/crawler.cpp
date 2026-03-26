#include "crawl/crawler.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <queue>
#include <unordered_set>

#include "crawl/html_extractor.h"

namespace glite {

std::string Crawler::DomainOf(const std::string& url) const {
  auto p = url.find("://");
  auto start = (p == std::string::npos) ? 0 : p + 3;
  auto slash = url.find('/', start);
  return url.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
}

std::string Crawler::FetchUrl(const std::string& url) const {
  std::string cmd = "curl -L -s --max-time 8 \"" + url + "\"";
  std::array<char, 4096> buf{};
  std::string out;
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return out;
  while (fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
    out += buf.data();
  }
  pclose(pipe);
  return out;
}

std::vector<Document> Crawler::Crawl(const std::vector<std::string>& seed_urls,
                                     const CrawlOptions& options) const {
  HtmlExtractor extractor;
  std::vector<Document> docs;
  if (seed_urls.empty()) return docs;
  const std::string seed_domain = DomainOf(seed_urls.front());
  std::queue<std::pair<std::string, std::size_t>> q;
  std::unordered_set<std::string> seen;
  for (const auto& u : seed_urls) q.push({u, 0});

  std::uint32_t next_id = 1;
  while (!q.empty() && docs.size() < options.max_pages) {
    auto [url, depth] = q.front();
    q.pop();
    if (seen.count(url)) continue;
    seen.insert(url);
    if (options.same_domain_only && DomainOf(url) != seed_domain) continue;

    const auto html = FetchUrl(url);
    if (html.empty()) continue;
    const auto page = extractor.Extract(html, url);
    if (page.text.empty()) continue;
    Document d;
    d.id = next_id++;
    d.url = url;
    d.title = page.title.empty() ? url : page.title;
    d.body = page.text;
    d.domain = DomainOf(url);
    d.language = "en";
    d.crawled_at_unix =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    docs.push_back(std::move(d));

    if (depth >= options.max_depth) continue;
    for (const auto& link : page.links) {
      if (!seen.count(link)) q.push({link, depth + 1});
    }
  }
  return docs;
}

}  // namespace glite
