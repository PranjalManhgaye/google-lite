#include <iostream>

#include "api/http_server.h"
#include "index/index_snapshot.h"
#include "io/doc_store.h"
#include "io/wiki_loader.h"
#include "search/opensearch_client.h"
#include "search/search_service.h"

int main(int argc, char** argv) {
  std::uint16_t port = argc > 1 ? static_cast<std::uint16_t>(std::stoi(argv[1])) : 8080;
  glite::IndexSnapshot snapshot("data/store");
  glite::DocStore store("data/store");
  glite::OpenSearchClient os("http://localhost:9200", "google_lite_docs");
  if (os.IsAvailable()) {
    os.EnsureIndex();
  }
  auto index = snapshot.LoadToIndex(true);
  if (index.DocCount() == 0) {
    glite::WikiLoader loader;
    auto docs = loader.LoadTsv("data/sample_wiki.tsv");
    for (const auto& d : docs) index.AddDocument(d);
    index.Finalize();
    snapshot.SaveFromIndex(index);
  }
  glite::SearchService service(&index);
  glite::HttpServer server(&service, &index, &snapshot, &store, &os);
  std::cout << "Search API listening on http://localhost:" << port << "\n";
  return server.Start(port) ? 0 : 1;
}
