#include "io/parallel_index_builder.h"

#include <thread>

namespace glite {

InvertedIndex ParallelIndexBuilder::Build(const std::vector<Document>& docs, std::size_t thread_count,
                                          bool enable_compression) {
  if (docs.empty()) return InvertedIndex(enable_compression);
  if (thread_count == 0) thread_count = std::max<std::size_t>(1, std::thread::hardware_concurrency());
  thread_count = std::min<std::size_t>(thread_count, docs.size());

  std::vector<InvertedIndex> partials;
  partials.reserve(thread_count);
  for (std::size_t i = 0; i < thread_count; ++i) {
    partials.emplace_back(false);
  }

  std::vector<std::thread> workers;
  const std::size_t chunk = (docs.size() + thread_count - 1) / thread_count;
  for (std::size_t t = 0; t < thread_count; ++t) {
    const std::size_t begin = t * chunk;
    const std::size_t end = std::min(docs.size(), begin + chunk);
    workers.emplace_back([begin, end, &docs, &partials, t]() {
      for (std::size_t i = begin; i < end; ++i) partials[t].AddDocument(docs[i]);
      partials[t].Finalize();
    });
  }
  for (auto& w : workers) w.join();

  InvertedIndex merged(enable_compression);
  for (const auto& idx : partials) {
    for (auto doc_id : idx.UniverseDocs()) {
      const auto* d = idx.GetDocument(doc_id);
      if (d) merged.AddDocument(*d);
    }
  }
  merged.Finalize();
  return merged;
}

}  // namespace glite
