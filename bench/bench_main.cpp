#include <chrono>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "io/parallel_index_builder.h"
#include "io/wiki_loader.h"
#include "search/search_service.h"
#include "search/search_types.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: bench_engine <dataset.tsv> [max_docs]\n";
    return 1;
  }
  std::size_t max_docs = argc > 2 ? static_cast<std::size_t>(std::stoull(argv[2])) : 5000;

  glite::WikiLoader loader;
  auto docs = loader.LoadTsv(argv[1], max_docs);
  if (docs.empty()) {
    std::cerr << "No docs loaded.\n";
    return 1;
  }

  auto t0 = std::chrono::steady_clock::now();
  auto index = glite::ParallelIndexBuilder::Build(docs, 0, true);
  auto t1 = std::chrono::steady_clock::now();
  auto index_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  double throughput = docs.empty() ? 0.0 : static_cast<double>(docs.size()) / (index_ms / 1000.0 + 1e-9);

  std::vector<std::string> queries = {
      "computer AND science", "\"new york\" AND city", "history OR culture", "NOT ancient"};
  glite::SearchService service(&index);
  glite::SearchOptions opts;
  opts.explain = true;
  std::vector<long long> latencies;
  std::vector<long long> cache_latencies;
  for (const auto& q : queries) {
    auto s = std::chrono::steady_clock::now();
    bool hit = false;
    auto r = service.Search(q, opts, &hit);
    (void)r;
    auto e = std::chrono::steady_clock::now();
    latencies.push_back(std::chrono::duration_cast<std::chrono::microseconds>(e - s).count());
    auto c0 = std::chrono::steady_clock::now();
    auto c = service.Search(q, opts, &hit);
    (void)c;
    auto c1 = std::chrono::steady_clock::now();
    cache_latencies.push_back(std::chrono::duration_cast<std::chrono::microseconds>(c1 - c0).count());
  }
  auto p50 = [&](std::vector<long long> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
  };
  auto p95 = [&](std::vector<long long> v) {
    std::sort(v.begin(), v.end());
    return v[static_cast<std::size_t>(0.95 * (v.size() - 1))];
  };

  std::cout << "docs=" << docs.size() << "\n";
  std::cout << "vocab=" << index.Vocabulary().size() << "\n";
  std::cout << "compressed_bytes=" << index.CompressedBytes() << "\n";
  std::cout << "index_ms=" << index_ms << "\n";
  std::cout << "index_throughput_docs_per_sec=" << throughput << "\n";
  std::cout << "p50_latency_us=" << p50(latencies) << "\n";
  std::cout << "p95_latency_us=" << p95(latencies) << "\n";
  std::cout << "cached_p50_latency_us=" << p50(cache_latencies) << "\n";
  std::cout << "cached_p95_latency_us=" << p95(cache_latencies) << "\n";
  for (std::size_t i = 0; i < queries.size(); ++i) {
    std::cout << "query_" << i << "_latency_us=" << latencies[i] << "\n";
  }
  return 0;
}
