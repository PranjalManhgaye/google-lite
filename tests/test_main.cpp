#include <cassert>
#include <iostream>
#include <string>

#include "index/inverted_index.h"
#include "index/index_snapshot.h"
#include "io/doc_store.h"
#include "search/search_service.h"
#include "search/search_types.h"

int main() {
  glite::InvertedIndex index(true);
  index.AddDocument({1, "New York City Guide", "Travel guide for new york city and food"});
  index.AddDocument({2, "Machine Learning Intro", "Computer science and machine learning basics"});
  index.AddDocument({3, "Ancient History", "Ancient empire and culture"});
  index.Finalize();

  glite::SearchService service(&index);
  glite::SearchOptions opts;
  opts.top_k = 5;
  opts.explain = true;

  auto phrase = service.Search("\"new york\"", opts);
  assert(!phrase.empty());
  assert(phrase[0].doc_id == 1);

  auto boolean = service.Search("computer AND science", opts);
  assert(!boolean.empty());
  assert(boolean[0].doc_id == 2);
  assert(!boolean[0].snippet.empty());

  opts.top_k = 10;
  auto not_q = service.Search("NOT ancient", opts);
  bool contains_3 = false;
  for (const auto& r : not_q) {
    if (r.doc_id == 3) contains_3 = true;
  }
  assert(!contains_3);

  opts.field_filter = glite::SearchFieldFilter::kTitle;
  auto title_only = service.Search("machine", opts);
  assert(!title_only.empty());
  assert(title_only[0].doc_id == 2);

  auto typo = service.TypoSuggestions("lerning");
  assert(!typo.empty());

  glite::DocStore store("data/test_store");
  store.SaveDocuments({glite::Document{99, "Crawler Doc", "This is crawled content"}});
  auto loaded_docs = store.LoadDocuments();
  assert(!loaded_docs.empty());
  assert(loaded_docs[0].id == 99);

  glite::IndexSnapshot snapshot("data/test_store");
  auto snap_index = snapshot.LoadToIndex(true);
  assert(snap_index.DocCount() >= 1);

  glite::Document url_doc;
  url_doc.id = 100;
  url_doc.url = "https://example.com";
  url_doc.title = "Example URL";
  url_doc.body = "Example body text";
  url_doc.domain = "example.com";
  std::vector<glite::Document> persisted = store.LoadDocuments();
  bool seen_url = false;
  for (const auto& d : persisted) {
    if (d.url == url_doc.url) seen_url = true;
  }
  if (!seen_url) {
    persisted.push_back(url_doc);
    store.SaveDocuments(persisted);
  }
  auto after = store.LoadDocuments();
  int url_count = 0;
  for (const auto& d : after) {
    if (d.url == "https://example.com") ++url_count;
  }
  assert(url_count == 1);

  std::cout << "All tests passed.\n";
  return 0;
}
