#include "io/doc_store.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace glite {

namespace {
std::string Escape(const std::string& s) {
  std::string out = s;
  for (char& c : out) {
    if (c == '\t' || c == '\n' || c == '\r') c = ' ';
  }
  return out;
}
}  // namespace

std::string DocStore::DocsPath() const { return root_dir_ + "/docs.tsv"; }
std::string DocStore::FrontierPath() const { return root_dir_ + "/frontier.txt"; }

bool DocStore::SaveDocuments(const std::vector<Document>& docs) const {
  std::filesystem::create_directories(root_dir_);
  std::ofstream out(DocsPath(), std::ios::trunc);
  if (!out) return false;
  for (const auto& d : docs) {
    out << d.id << "\t" << Escape(d.url) << "\t" << Escape(d.title) << "\t" << Escape(d.body) << "\t"
        << Escape(d.domain) << "\t" << Escape(d.language) << "\t" << d.crawled_at_unix << "\n";
  }
  return true;
}

std::vector<Document> DocStore::LoadDocuments() const {
  std::ifstream in(DocsPath());
  std::vector<Document> docs;
  if (!in) return docs;
  std::string line;
  while (std::getline(in, line)) {
    std::stringstream ss(line);
    std::string id, url, title, body, domain, language, ts;
    if (!std::getline(ss, id, '\t') || !std::getline(ss, url, '\t') || !std::getline(ss, title, '\t') ||
        !std::getline(ss, body, '\t') || !std::getline(ss, domain, '\t') || !std::getline(ss, language, '\t') ||
        !std::getline(ss, ts)) {
      continue;
    }
    Document d;
    d.id = static_cast<std::uint32_t>(std::stoul(id));
    d.url = url;
    d.title = title;
    d.body = body;
    d.domain = domain;
    d.language = language;
    d.crawled_at_unix = std::stoll(ts);
    docs.push_back(std::move(d));
  }
  return docs;
}

bool DocStore::SaveFrontier(const std::vector<std::string>& urls) const {
  std::filesystem::create_directories(root_dir_);
  std::ofstream out(FrontierPath(), std::ios::trunc);
  if (!out) return false;
  for (const auto& u : urls) out << u << "\n";
  return true;
}

std::vector<std::string> DocStore::LoadFrontier() const {
  std::ifstream in(FrontierPath());
  std::vector<std::string> urls;
  if (!in) return urls;
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty()) urls.push_back(line);
  }
  return urls;
}

}  // namespace glite
