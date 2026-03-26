#include <iostream>

#include "crawl/crawler.h"
#include "index/index_snapshot.h"
#include "io/doc_store.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: crawl_index_tool <seed_url> [max_pages]\n";
    return 1;
  }
  std::size_t max_pages = argc > 2 ? static_cast<std::size_t>(std::stoull(argv[2])) : 20;
  glite::Crawler crawler;
  glite::CrawlOptions opts;
  opts.max_pages = max_pages;
  opts.max_depth = 1;

  auto docs = crawler.Crawl({argv[1]}, opts);
  if (docs.empty()) {
    std::cerr << "No documents crawled.\n";
    return 1;
  }
  glite::DocStore store("data/store");
  store.SaveDocuments(docs);
  store.SaveFrontier({argv[1]});

  glite::IndexSnapshot snapshot("data/store");
  auto index = snapshot.IncrementalRebuild(docs, true);
  std::cout << "Crawled docs=" << docs.size() << " indexed_docs=" << index.DocCount()
            << " vocab=" << index.Vocabulary().size() << "\n";
  return 0;
}
