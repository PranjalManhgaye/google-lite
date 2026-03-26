#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/document.h"
#include "core/normalizer.h"
#include "core/tokenizer.h"

namespace glite {

struct Posting {
  std::uint32_t doc_id{};
  std::vector<std::uint32_t> positions;
  std::uint32_t skip_to{0};
  bool has_skip{false};
};

class InvertedIndex {
 public:
  explicit InvertedIndex(bool enable_compression = false) : enable_compression_(enable_compression) {}
  void AddDocument(const Document& doc);
  void Finalize();

  const std::vector<Posting>* GetPostings(const std::string& term) const;
  std::vector<std::uint32_t> UniverseDocs() const;
  const Document* GetDocument(std::uint32_t doc_id) const;
  std::uint32_t DocLength(std::uint32_t doc_id) const;
  double AvgDocLength() const;
  std::uint32_t DocFreq(const std::string& term) const;
  std::uint32_t CollectionTermFreq(const std::string& term) const;
  const std::unordered_map<std::string, std::uint32_t>& CollectionTermFreqMap() const;
  std::size_t DocCount() const;
  std::vector<std::string> Vocabulary() const;
  std::size_t CompressedBytes() const;

 private:
  void BuildSkips();
  void EncodeCompressedPostings();
  void VarintPush(std::uint32_t value, std::vector<std::uint8_t>* out) const;
  std::unordered_map<std::string, std::vector<Posting>> postings_;
  std::unordered_map<std::string, std::uint32_t> collection_freq_;
  std::unordered_map<std::string, std::vector<std::uint8_t>> compressed_postings_;
  std::unordered_map<std::uint32_t, Document> docs_;
  std::unordered_map<std::uint32_t, std::uint32_t> doc_lengths_;
  std::unordered_set<std::uint32_t> seen_doc_ids_;
  Tokenizer tokenizer_;
  Normalizer normalizer_;
  bool enable_compression_{false};
  std::size_t compressed_bytes_{0};
  double avg_doc_len_{0.0};
};

}  // namespace glite
