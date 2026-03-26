#include "search/opensearch_client.h"

#include <array>
#include <cstdio>
#include <functional>
#include <regex>

namespace glite {

std::string OpenSearchClient::JsonEscape(const std::string& s) const {
  std::string out;
  for (char c : s) {
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else out += c;
  }
  return out;
}

std::string OpenSearchClient::UrlKey(const std::string& url) const {
  return std::to_string(std::hash<std::string>{}(url));
}

std::string OpenSearchClient::Curl(const std::string& method, const std::string& path,
                                   const std::string& body) const {
  std::string cmd = "curl -s -X " + method + " '" + base_url_ + "/" + index_name_ + path + "'";
  if (!body.empty()) {
    cmd += " -H 'Content-Type: application/json' -d '" + body + "'";
  }
  std::array<char, 4096> buf{};
  std::string out;
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return out;
  while (fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) out += buf.data();
  pclose(pipe);
  return out;
}

bool OpenSearchClient::IsAvailable() const {
  std::string cmd = "curl -s --max-time 2 '" + base_url_ + "/_cluster/health'";
  std::array<char, 1024> buf{};
  std::string out;
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return false;
  while (fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) out += buf.data();
  pclose(pipe);
  return out.find("\"status\"") != std::string::npos;
}

bool OpenSearchClient::EnsureIndex() {
  std::string mapping =
      "{\"mappings\":{\"properties\":{\"url\":{\"type\":\"keyword\"},\"title\":{\"type\":\"text\"},"
      "\"body\":{\"type\":\"text\"},\"domain\":{\"type\":\"keyword\"},\"crawled_at\":{\"type\":\"long\"}}}}";
  auto out = Curl("PUT", "", mapping);
  return out.find("\"acknowledged\"") != std::string::npos || out.find("resource_already_exists_exception") != std::string::npos;
}

bool OpenSearchClient::UpsertByUrl(const Document& doc) {
  std::string id = UrlKey(doc.url);
  std::string body = "{\"doc\":{\"url\":\"" + JsonEscape(doc.url) + "\",\"title\":\"" + JsonEscape(doc.title) +
                     "\",\"body\":\"" + JsonEscape(doc.body) + "\",\"domain\":\"" + JsonEscape(doc.domain) +
                     "\",\"crawled_at\":" + std::to_string(doc.crawled_at_unix) + "},\"doc_as_upsert\":true}";
  auto out = Curl("POST", "/_update/" + id, body);
  return out.find("\"result\"") != std::string::npos;
}

std::string OpenSearchClient::ExtractJsonField(const std::string& json, const std::string& key) const {
  std::regex re("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
  std::smatch m;
  if (std::regex_search(json, m, re)) return m[1].str();
  return {};
}

double OpenSearchClient::ExtractScore(const std::string& json) const {
  std::regex re("\"_score\"\\s*:\\s*([0-9eE+\\.-]+)");
  std::smatch m;
  if (std::regex_search(json, m, re)) return std::stod(m[1].str());
  return 0.0;
}

bool OpenSearchClient::GetByUrl(const std::string& url, OpenSearchHit* hit) {
  std::string id = UrlKey(url);
  return GetById(id, hit);
}

bool OpenSearchClient::GetById(const std::string& id, OpenSearchHit* hit) {
  auto out = Curl("GET", "/_doc/" + id);
  if (out.find("\"found\":true") == std::string::npos) return false;
  hit->id = id;
  hit->url = ExtractJsonField(out, "url");
  hit->title = ExtractJsonField(out, "title");
  hit->snippet = ExtractJsonField(out, "body");
  if (hit->snippet.size() > 240) hit->snippet.resize(240);
  hit->score = 1.0;
  return true;
}

std::vector<OpenSearchHit> OpenSearchClient::Search(const std::string& query, std::size_t top_k) {
  std::string body = "{\"size\":" + std::to_string(top_k) +
                     ",\"query\":{\"multi_match\":{\"query\":\"" + JsonEscape(query) +
                     "\",\"fields\":[\"title^2\",\"body\"]}}}";
  auto out = Curl("GET", "/_search", body);
  std::vector<OpenSearchHit> hits;
  if (out.find("\"hits\"") == std::string::npos) return hits;
  std::regex block("\\{\"_index\"[\\s\\S]*?\\}\\}");
  auto begin = std::sregex_iterator(out.begin(), out.end(), block);
  auto end = std::sregex_iterator();
  for (auto it = begin; it != end && hits.size() < top_k; ++it) {
    OpenSearchHit h;
    auto chunk = it->str();
    h.id = ExtractJsonField(chunk, "_id");
    h.url = ExtractJsonField(chunk, "url");
    h.title = ExtractJsonField(chunk, "title");
    h.snippet = ExtractJsonField(chunk, "body");
    if (h.snippet.size() > 240) h.snippet.resize(240);
    h.score = ExtractScore(chunk);
    hits.push_back(std::move(h));
  }
  return hits;
}

std::vector<std::string> OpenSearchClient::Suggest(const std::string& query, std::size_t top_k) {
  auto hits = Search(query, top_k);
  std::vector<std::string> out;
  for (const auto& h : hits) {
    if (!h.title.empty()) out.push_back(h.title);
  }
  return out;
}

}  // namespace glite
