#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include "index/index_snapshot.h"
#include "io/wiki_loader.h"
#include "search/search_service.h"
#include "search/search_types.h"

int main() {
  glite::IndexSnapshot snapshot("data/store");
  auto index = snapshot.LoadToIndex(true);
  if (index.DocCount() == 0) {
    glite::WikiLoader loader;
    auto docs = loader.LoadTsv("data/sample_wiki.tsv");
    for (const auto& d : docs) index.AddDocument(d);
    index.Finalize();
  }
  glite::SearchService service(&index);
  glite::SearchOptions opts;
  opts.explain = false;
  std::vector<std::string> queries = {"machine learning", "\"new york\" city", "history OR culture", "NOT ancient"};
  std::vector<long long> lat;
  for (const auto& q : queries) {
    auto s = std::chrono::steady_clock::now();
    bool hit = false;
    auto r = service.Search(q, opts, &hit);
    (void)r;
    auto e = std::chrono::steady_clock::now();
    lat.push_back(std::chrono::duration_cast<std::chrono::microseconds>(e - s).count());
  }
  std::sort(lat.begin(), lat.end());
  std::cout << "api_p50_us=" << lat[lat.size() / 2] << "\n";
  std::cout << "api_p95_us=" << lat[static_cast<std::size_t>(0.95 * (lat.size() - 1))] << "\n";
  return 0;
}
