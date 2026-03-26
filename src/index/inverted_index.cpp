#include "index/inverted_index.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace glite {

void InvertedIndex::AddDocument(const Document& doc) {
  if (seen_doc_ids_.find(doc.id) != seen_doc_ids_.end()) {
    return;
  }
  seen_doc_ids_.insert(doc.id);
  docs_[doc.id] = doc;
  const auto title_tokens = tokenizer_.Split(doc.title);
  const auto body_tokens = tokenizer_.Split(doc.body);

  std::uint32_t position = 0;
  std::uint32_t kept_terms = 0;
  auto consume = [&](const std::vector<std::string>& tokens) {
    for (const auto& raw : tokens) {
      const std::string token = normalizer_.NormalizeToken(raw);
      if (token.empty() || normalizer_.IsStopword(token)) {
        continue;
      }
      auto& plist = postings_[token];
      if (plist.empty() || plist.back().doc_id != doc.id) {
        plist.push_back(Posting{doc.id, {}});
      }
      plist.back().positions.push_back(position++);
      ++collection_freq_[token];
      ++kept_terms;
    }
  };

  consume(title_tokens);
  consume(body_tokens);
  doc_lengths_[doc.id] = kept_terms;
}

void InvertedIndex::Finalize() {
  if (doc_lengths_.empty()) {
    avg_doc_len_ = 0.0;
    return;
  }
  std::uint64_t sum = 0;
  for (const auto& [_, len] : doc_lengths_) {
    sum += len;
  }
  avg_doc_len_ = static_cast<double>(sum) / static_cast<double>(doc_lengths_.size());
  BuildSkips();
  if (enable_compression_) {
    EncodeCompressedPostings();
  }
}

const std::vector<Posting>* InvertedIndex::GetPostings(const std::string& term) const {
  auto it = postings_.find(normalizer_.NormalizeToken(term));
  if (it == postings_.end()) {
    return nullptr;
  }
  return &it->second;
}

std::vector<std::uint32_t> InvertedIndex::UniverseDocs() const {
  std::vector<std::uint32_t> docs;
  docs.reserve(docs_.size());
  for (const auto& [id, _] : docs_) {
    docs.push_back(id);
  }
  std::sort(docs.begin(), docs.end());
  return docs;
}

const Document* InvertedIndex::GetDocument(std::uint32_t doc_id) const {
  auto it = docs_.find(doc_id);
  if (it == docs_.end()) {
    return nullptr;
  }
  return &it->second;
}

std::uint32_t InvertedIndex::DocLength(std::uint32_t doc_id) const {
  auto it = doc_lengths_.find(doc_id);
  return it == doc_lengths_.end() ? 0 : it->second;
}

double InvertedIndex::AvgDocLength() const { return avg_doc_len_; }

std::uint32_t InvertedIndex::DocFreq(const std::string& term) const {
  auto it = postings_.find(normalizer_.NormalizeToken(term));
  return it == postings_.end() ? 0U : static_cast<std::uint32_t>(it->second.size());
}

std::uint32_t InvertedIndex::CollectionTermFreq(const std::string& term) const {
  auto it = collection_freq_.find(normalizer_.NormalizeToken(term));
  return it == collection_freq_.end() ? 0U : it->second;
}

const std::unordered_map<std::string, std::uint32_t>& InvertedIndex::CollectionTermFreqMap() const {
  return collection_freq_;
}

std::size_t InvertedIndex::DocCount() const { return docs_.size(); }

std::vector<std::string> InvertedIndex::Vocabulary() const {
  std::vector<std::string> terms;
  terms.reserve(postings_.size());
  for (const auto& [term, _] : postings_) {
    terms.push_back(term);
  }
  std::sort(terms.begin(), terms.end());
  return terms;
}

std::size_t InvertedIndex::CompressedBytes() const { return compressed_bytes_; }

void InvertedIndex::BuildSkips() {
  for (auto& [_, plist] : postings_) {
    if (plist.size() < 4) continue;
    const std::size_t step = static_cast<std::size_t>(std::sqrt(static_cast<double>(plist.size())));
    if (step < 2) continue;
    for (std::size_t i = 0; i + step < plist.size(); i += step) {
      plist[i].skip_to = static_cast<std::uint32_t>(i + step);
      plist[i].has_skip = true;
    }
  }
}

void InvertedIndex::VarintPush(std::uint32_t value, std::vector<std::uint8_t>* out) const {
  while (value >= 0x80U) {
    out->push_back(static_cast<std::uint8_t>(value | 0x80U));
    value >>= 7;
  }
  out->push_back(static_cast<std::uint8_t>(value));
}

void InvertedIndex::EncodeCompressedPostings() {
  compressed_postings_.clear();
  compressed_bytes_ = 0;
  for (const auto& [term, plist] : postings_) {
    std::vector<std::uint8_t> bytes;
    std::uint32_t prev_doc = 0;
    for (const auto& posting : plist) {
      const std::uint32_t doc_gap = posting.doc_id - prev_doc;
      VarintPush(doc_gap, &bytes);
      prev_doc = posting.doc_id;
      VarintPush(static_cast<std::uint32_t>(posting.positions.size()), &bytes);
      std::uint32_t prev_pos = 0;
      for (auto pos : posting.positions) {
        VarintPush(pos - prev_pos, &bytes);
        prev_pos = pos;
      }
    }
    compressed_bytes_ += bytes.size();
    compressed_postings_[term] = std::move(bytes);
  }
}

}  // namespace glite
