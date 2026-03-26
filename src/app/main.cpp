#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "io/parallel_index_builder.h"
#include "io/wiki_loader.h"
#include "search/search_service.h"
#include "search/search_types.h"

namespace {
void ClearScreen() { std::cout << "\033[2J\033[H"; }

std::string FilterName(glite::SearchFieldFilter f) {
  if (f == glite::SearchFieldFilter::kTitle) return "title";
  if (f == glite::SearchFieldFilter::kBody) return "body";
  return "all";
}

std::string SortName(glite::SortMode s) {
  if (s == glite::SortMode::kTitle) return "title";
  if (s == glite::SortMode::kLength) return "length";
  return "relevance";
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: search_cli <dataset.tsv> [max_docs] [threads]\n";
    return 1;
  }

  std::size_t max_docs = 0;
  if (argc >= 3) max_docs = static_cast<std::size_t>(std::stoull(argv[2]));
  std::size_t threads = 0;
  if (argc >= 4) threads = static_cast<std::size_t>(std::stoull(argv[3]));

  glite::WikiLoader loader;
  auto docs = loader.LoadTsv(argv[1], max_docs);
  if (docs.empty()) {
    std::cerr << "No documents loaded.\n";
    return 1;
  }

  auto index = glite::ParallelIndexBuilder::Build(docs, threads, true);
  glite::SearchService service(&index);

  glite::SearchOptions options;
  options.top_k = 10;
  options.explain = true;
  std::string query = "machine AND learning";
  std::vector<glite::SearchResult> results;
  std::vector<std::string> suggestions;
  std::size_t selected = 0;
  long long last_latency_us = 0;
  bool last_cache_hit = false;
  std::string status = "ready";

  for (;;) {
    ClearScreen();
    std::cout << "Google Lite TUI Search Engine\n";
    std::cout << "docs=" << index.DocCount() << " vocab=" << index.Vocabulary().size()
              << " compressed_bytes=" << index.CompressedBytes() << "\n";
    std::cout << "query: " << query << "\n";
    std::cout << "filter=" << FilterName(options.field_filter)
              << " sort=" << SortName(options.sort_mode)
              << " explain=" << (options.explain ? "on" : "off")
              << " cache=" << (last_cache_hit ? "hit" : "miss")
              << " latency_us=" << last_latency_us << "\n";
    std::cout << "commands: /new query | j/k select | f all|title|body | s relevance|title|length | e | t term | q\n";
    std::cout << "------------------------------------------------------------\n";
    suggestions = service.LiveSuggestions(query);
    std::cout << "suggestions: ";
    for (const auto& s : suggestions) std::cout << s << " ";
    std::cout << "\n------------------------------------------------------------\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
      std::cout << (i == selected ? "> " : "  ") << "[" << results[i].doc_id << "] "
                << results[i].title << " score=" << results[i].score << "\n";
      std::cout << "    " << results[i].snippet << "\n";
    }
    if (!results.empty() && selected < results.size() && !results[selected].explain_terms.empty()) {
      std::cout << "------------------------------------------------------------\n";
      std::cout << "detail (term contributions): ";
      for (const auto& [term, score] : results[selected].explain_terms) {
        std::cout << term << "=" << score << " ";
      }
      std::cout << "\n";
    }
    std::cout << "status: " << status << "\n";
    std::cout << "cmd> ";
    std::string line;
    if (!std::getline(std::cin, line)) break;
    if (line == "q") break;
    if (line == "j") {
      if (selected + 1 < results.size()) ++selected;
      continue;
    }
    if (line == "k") {
      if (selected > 0) --selected;
      continue;
    }
    if (line == "e") {
      options.explain = !options.explain;
      continue;
    }
    if (line.rfind("f ", 0) == 0) {
      std::string v = line.substr(2);
      if (v == "title") options.field_filter = glite::SearchFieldFilter::kTitle;
      else if (v == "body") options.field_filter = glite::SearchFieldFilter::kBody;
      else options.field_filter = glite::SearchFieldFilter::kAll;
      continue;
    }
    if (line.rfind("s ", 0) == 0) {
      std::string v = line.substr(2);
      if (v == "title") options.sort_mode = glite::SortMode::kTitle;
      else if (v == "length") options.sort_mode = glite::SortMode::kLength;
      else options.sort_mode = glite::SortMode::kRelevance;
      continue;
    }
    if (line.rfind("t ", 0) == 0) {
      auto typo = service.TypoSuggestions(line.substr(2));
      status = "typo: ";
      for (const auto& s : typo) status += s + " ";
      continue;
    }
    if (line.rfind("/", 0) == 0) {
      query = line.substr(1);
    } else if (!line.empty()) {
      query = line;
    }
    auto start = std::chrono::steady_clock::now();
    results = service.Search(query, options, &last_cache_hit);
    auto end = std::chrono::steady_clock::now();
    last_latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    selected = std::min<std::size_t>(selected, results.empty() ? 0 : (results.size() - 1));
    status = "results=" + std::to_string(results.size());
  }
  return 0;
}
