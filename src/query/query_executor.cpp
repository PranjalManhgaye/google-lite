#include "query/query_executor.h"

#include <algorithm>
#include <cmath>
#include <stack>

#include "core/normalizer.h"
#include "core/tokenizer.h"

namespace glite {

std::vector<std::uint32_t> QueryExecutor::EvalTerm(const InvertedIndex& index,
                                                   const std::string& term) const {
  const auto* postings = index.GetPostings(term);
  if (postings == nullptr) return {};
  std::vector<std::uint32_t> out;
  out.reserve(postings->size());
  for (const auto& p : *postings) out.push_back(p.doc_id);
  return out;
}

std::vector<std::uint32_t> QueryExecutor::EvalPhrase(const InvertedIndex& index,
                                                     const std::string& phrase) const {
  Tokenizer tokenizer;
  Normalizer normalizer;
  auto toks = tokenizer.Split(phrase);
  std::vector<std::string> terms;
  for (const auto& tok : toks) {
    auto n = normalizer.NormalizeToken(tok);
    if (!n.empty() && !normalizer.IsStopword(n)) terms.push_back(n);
  }
  if (terms.empty()) return {};
  if (terms.size() == 1) return EvalTerm(index, terms.front());

  std::vector<const std::vector<Posting>*> plists;
  for (const auto& t : terms) {
    const auto* p = index.GetPostings(t);
    if (p == nullptr) return {};
    plists.push_back(p);
  }

  std::vector<std::uint32_t> docs = EvalTerm(index, terms[0]);
  for (std::size_t i = 1; i < terms.size(); ++i) {
    docs = SetAnd(docs, EvalTerm(index, terms[i]));
  }

  std::vector<std::uint32_t> phrase_hits;
  for (auto doc : docs) {
    std::vector<const Posting*> postings_for_doc;
    bool found_all = true;
    for (const auto* plist : plists) {
      auto it = std::lower_bound(plist->begin(), plist->end(), doc,
                                 [](const Posting& p, std::uint32_t d) { return p.doc_id < d; });
      if (it == plist->end() || it->doc_id != doc) {
        found_all = false;
        break;
      }
      postings_for_doc.push_back(&(*it));
    }
    if (!found_all) continue;

    bool phrase_found = false;
    for (auto pos0 : postings_for_doc[0]->positions) {
      bool ok = true;
      for (std::size_t i = 1; i < postings_for_doc.size(); ++i) {
        if (!std::binary_search(postings_for_doc[i]->positions.begin(),
                                postings_for_doc[i]->positions.end(), pos0 + i)) {
          ok = false;
          break;
        }
      }
      if (ok) {
        phrase_found = true;
        break;
      }
    }
    if (phrase_found) phrase_hits.push_back(doc);
  }
  return phrase_hits;
}

std::vector<std::uint32_t> QueryExecutor::SetAnd(const std::vector<std::uint32_t>& a,
                                                 const std::vector<std::uint32_t>& b) const {
  std::vector<std::uint32_t> out;
  std::size_t i = 0;
  std::size_t j = 0;
  const std::size_t skip_a = std::max<std::size_t>(2, static_cast<std::size_t>(std::sqrt(a.size())));
  const std::size_t skip_b = std::max<std::size_t>(2, static_cast<std::size_t>(std::sqrt(b.size())));
  while (i < a.size() && j < b.size()) {
    if (a[i] == b[j]) {
      out.push_back(a[i]);
      ++i;
      ++j;
    } else if (a[i] < b[j]) {
      if (i + skip_a < a.size() && a[i + skip_a] <= b[j]) i += skip_a;
      else ++i;
    } else {
      if (j + skip_b < b.size() && b[j + skip_b] <= a[i]) j += skip_b;
      else ++j;
    }
  }
  return out;
}

std::vector<std::uint32_t> QueryExecutor::SetOr(const std::vector<std::uint32_t>& a,
                                                const std::vector<std::uint32_t>& b) const {
  std::vector<std::uint32_t> out;
  std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
  return out;
}

std::vector<std::uint32_t> QueryExecutor::SetNot(const std::vector<std::uint32_t>& universe,
                                                 const std::vector<std::uint32_t>& b) const {
  std::vector<std::uint32_t> out;
  std::set_difference(universe.begin(), universe.end(), b.begin(), b.end(), std::back_inserter(out));
  return out;
}

std::vector<std::string> QueryExecutor::ExtractScoringTerms(const std::vector<QueryToken>& rpn) const {
  Tokenizer tokenizer;
  Normalizer normalizer;
  std::vector<std::string> terms;
  for (const auto& tok : rpn) {
    if (tok.type == QueryTokenType::kTerm) {
      auto n = normalizer.NormalizeToken(tok.value);
      if (!n.empty() && !normalizer.IsStopword(n)) terms.push_back(n);
    } else if (tok.type == QueryTokenType::kPhrase) {
      auto phrase_toks = tokenizer.Split(tok.value);
      for (const auto& pt : phrase_toks) {
        auto n = normalizer.NormalizeToken(pt);
        if (!n.empty() && !normalizer.IsStopword(n)) terms.push_back(n);
      }
    }
  }
  return terms;
}

std::vector<std::uint32_t> QueryExecutor::ApplyFieldFilter(const InvertedIndex* index,
                                                           const std::vector<std::uint32_t>& candidates,
                                                           SearchFieldFilter filter) const {
  if (filter == SearchFieldFilter::kAll) return candidates;
  Tokenizer tokenizer;
  Normalizer normalizer;
  std::vector<std::uint32_t> filtered;
  for (auto id : candidates) {
    const auto* doc = index->GetDocument(id);
    if (!doc) continue;
    const auto source = filter == SearchFieldFilter::kTitle ? doc->title : doc->body;
    const auto toks = tokenizer.Split(source);
    bool has_term = false;
    for (const auto& tk : toks) {
      const auto n = normalizer.NormalizeToken(tk);
      if (!n.empty() && !normalizer.IsStopword(n)) {
        has_term = true;
        break;
      }
    }
    if (has_term) filtered.push_back(id);
  }
  return filtered;
}

std::string QueryExecutor::Highlight(const std::string& text, const std::vector<std::string>& terms) const {
  std::string out = text;
  for (const auto& t : terms) {
    if (t.empty()) continue;
    std::size_t pos = 0;
    while ((pos = out.find(t, pos)) != std::string::npos) {
      out.replace(pos, t.size(), "\033[1;32m" + t + "\033[0m");
      pos += t.size() + 9;
    }
  }
  return out;
}

std::string QueryExecutor::BuildSnippet(const InvertedIndex* index, std::uint32_t doc_id,
                                        const std::vector<std::string>& terms) const {
  const auto* doc = index->GetDocument(doc_id);
  if (!doc) return {};
  const std::string& body = doc->body;
  std::size_t hit = std::string::npos;
  for (const auto& t : terms) {
    auto p = body.find(t);
    if (p != std::string::npos && (hit == std::string::npos || p < hit)) hit = p;
  }
  std::size_t start = hit == std::string::npos ? 0 : (hit > 50 ? hit - 50 : 0);
  std::size_t len = std::min<std::size_t>(160, body.size() - start);
  std::string snippet = body.substr(start, len);
  return Highlight((start > 0 ? "... " : "") + snippet + (start + len < body.size() ? " ..." : ""), terms);
}

SearchResponse QueryExecutor::Search(const InvertedIndex* index, const std::string& query,
                                     const SearchOptions& options) const {
  const auto rpn = parser_.ParseToRpn(query);
  if (rpn.empty()) return {};

  std::stack<std::vector<std::uint32_t>> st;
  const auto universe = index->UniverseDocs();

  for (const auto& tok : rpn) {
    if (tok.type == QueryTokenType::kTerm) {
      st.push(EvalTerm(*index, tok.value));
    } else if (tok.type == QueryTokenType::kPhrase) {
      st.push(EvalPhrase(*index, tok.value));
    } else if (tok.type == QueryTokenType::kNot) {
      if (st.empty()) return {};
      auto rhs = st.top();
      st.pop();
      st.push(SetNot(universe, rhs));
    } else if (tok.type == QueryTokenType::kAnd || tok.type == QueryTokenType::kOr) {
      if (st.size() < 2) return {};
      auto rhs = st.top();
      st.pop();
      auto lhs = st.top();
      st.pop();
      st.push(tok.type == QueryTokenType::kAnd ? SetAnd(lhs, rhs) : SetOr(lhs, rhs));
    }
  }
  if (st.empty()) return {};
  auto candidates = st.top();
  std::sort(candidates.begin(), candidates.end());
  candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
  candidates = ApplyFieldFilter(index, candidates, options.field_filter);

  const auto terms = ExtractScoringTerms(rpn);
  const auto score_bundle = ranker_.Score(*index, terms, candidates, options.explain);
  std::vector<SearchResult> results;
  results.reserve(score_bundle.scores.size());
  for (const auto& [doc, score] : score_bundle.scores) {
    const auto* d = index->GetDocument(doc);
    if (!d) continue;
    SearchResult r;
    r.doc_id = doc;
    r.score = score;
    r.title = Highlight(d->title, terms);
    r.snippet = BuildSnippet(index, doc, terms);
    if (options.explain) {
      auto it = score_bundle.term_contributions.find(doc);
      if (it != score_bundle.term_contributions.end()) r.explain_terms = it->second;
    }
    results.push_back(std::move(r));
  }
  if (options.sort_mode == SortMode::kTitle) {
    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) { return a.title < b.title; });
  } else if (options.sort_mode == SortMode::kLength) {
    std::sort(results.begin(), results.end(), [](const SearchResult& a, const SearchResult& b) {
      return a.snippet.size() < b.snippet.size();
    });
  } else {
    std::sort(results.begin(), results.end(), [](const SearchResult& a, const SearchResult& b) {
      if (a.score == b.score) return a.doc_id < b.doc_id;
      return a.score > b.score;
    });
  }
  if (results.size() > options.top_k) {
    results.resize(options.top_k);
  }
  return {results, terms};
}

}  // namespace glite
