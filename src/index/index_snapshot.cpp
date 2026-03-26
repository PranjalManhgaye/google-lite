#include "index/index_snapshot.h"

#include <unordered_map>

namespace glite {

bool IndexSnapshot::SaveFromIndex(const InvertedIndex& index) const {
  std::vector<Document> docs;
  for (auto id : index.UniverseDocs()) {
    const auto* d = index.GetDocument(id);
    if (d) docs.push_back(*d);
  }
  return store_.SaveDocuments(docs);
}

InvertedIndex IndexSnapshot::LoadToIndex(bool enable_compression) const {
  InvertedIndex index(enable_compression);
  auto docs = store_.LoadDocuments();
  for (const auto& d : docs) index.AddDocument(d);
  index.Finalize();
  return index;
}

InvertedIndex IndexSnapshot::IncrementalRebuild(const std::vector<Document>& new_docs,
                                                bool enable_compression) const {
  auto existing = store_.LoadDocuments();
  std::unordered_map<std::uint32_t, Document> merged;
  for (const auto& d : existing) merged[d.id] = d;
  for (const auto& d : new_docs) merged[d.id] = d;

  std::vector<Document> docs;
  docs.reserve(merged.size());
  for (const auto& [_, d] : merged) docs.push_back(d);
  store_.SaveDocuments(docs);

  InvertedIndex index(enable_compression);
  for (const auto& d : docs) index.AddDocument(d);
  index.Finalize();
  return index;
}

}  // namespace glite
