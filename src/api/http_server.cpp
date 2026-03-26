#include "api/http_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <array>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <regex>
#include <sstream>
#include <cstdio>
#include <iomanip>

#include "crawl/html_extractor.h"

namespace glite {
namespace {
std::string ExtractField(const std::string& json, const std::string& key) {
  std::regex re("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
  std::smatch m;
  if (std::regex_search(json, m, re)) return m[1].str();
  return {};
}
}  // namespace

std::string HttpServer::JsonEscape(const std::string& s) const {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else out += c;
  }
  return out;
}

std::string HttpServer::Response(const std::string& body, const std::string& content_type,
                                 const std::string& status) {
  std::ostringstream oss;
  oss << "HTTP/1.1 " << status << "\r\n";
  oss << "Content-Type: " << content_type << "\r\n";
  oss << "Content-Length: " << body.size() << "\r\n";
  oss << "Access-Control-Allow-Origin: *\r\n";
  oss << "Connection: close\r\n\r\n";
  oss << body;
  return oss.str();
}

std::string HttpServer::ReadFileText(const std::string& path) const {
  std::ifstream in(path);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::string HttpServer::UrlDecode(const std::string& s) const {
  std::string out;
  out.reserve(s.size());
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '+' ) {
      out.push_back(' ');
    } else if (s[i] == '%' && i + 2 < s.size()) {
      const std::string hex = s.substr(i + 1, 2);
      char c = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
      out.push_back(c);
      i += 2;
    } else {
      out.push_back(s[i]);
    }
  }
  return out;
}

std::string HttpServer::QueryParam(const std::string& path, const std::string& key) const {
  auto qmark = path.find('?');
  if (qmark == std::string::npos) return {};
  std::string query = path.substr(qmark + 1);
  std::stringstream ss(query);
  std::string pair;
  while (std::getline(ss, pair, '&')) {
    auto eq = pair.find('=');
    if (eq == std::string::npos) continue;
    auto k = pair.substr(0, eq);
    auto v = pair.substr(eq + 1);
    if (k == key) return UrlDecode(v);
  }
  return {};
}

std::string HttpServer::HandleOpenSearchXml() {
  std::ostringstream xml;
  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xml << "<OpenSearchDescription xmlns=\"http://a9.com/-/spec/opensearch/1.1/\">\n";
  xml << "  <ShortName>Google Lite</ShortName>\n";
  xml << "  <Description>Google Lite Search</Description>\n";
  xml << "  <InputEncoding>UTF-8</InputEncoding>\n";
  xml << "  <Url type=\"text/html\" template=\"http://localhost:8080/search?q={searchTerms}\"/>\n";
  xml << "  <Url type=\"application/x-suggestions+json\" template=\"http://localhost:8080/suggest?q={searchTerms}\"/>\n";
  xml << "</OpenSearchDescription>\n";
  return Response(xml.str(), "application/opensearchdescription+xml");
}

bool HttpServer::IsUrlQuery(const std::string& q) const {
  return q.rfind("http://", 0) == 0 || q.rfind("https://", 0) == 0;
}

bool HttpServer::RebuildIndexFromStore() {
  if (!store_ || !snapshot_) return false;
  auto rebuilt = snapshot_->LoadToIndex(true);
  *index_ = std::move(rebuilt);
  *service_ = SearchService(index_);
  return true;
}

std::string HttpServer::HandleUrlPreview(const std::string& url) {
  if (os_client_ && os_client_->IsAvailable()) {
    OpenSearchHit existing;
    if (os_client_->GetByUrl(url, &existing)) {
      std::ostringstream json;
      json << "{\"mode\":\"url_preview\",\"backend\":\"opensearch\",\"dedup\":true,\"url\":\"" << JsonEscape(url)
           << "\",\"doc_id\":\"" << JsonEscape(existing.id) << "\",\"title\":\"" << JsonEscape(existing.title)
           << "\",\"snippet\":\"" << JsonEscape(existing.snippet) << "\"}";
      return Response(json.str());
    }
  }
  if (!store_ || !snapshot_) {
    return Response("{\"error\":\"store not configured\"}", "application/json", "500 Internal Server Error");
  }
  auto docs = store_->LoadDocuments();
  for (const auto& d : docs) {
    if (d.url == url) {
      RebuildIndexFromStore();
      std::ostringstream cached;
      cached << "{\"mode\":\"url_preview\",\"dedup\":true,\"url\":\"" << JsonEscape(url)
             << "\",\"doc_id\":" << d.id << "}";
      return Response(cached.str());
    }
  }

  std::string cmd = "curl -L -s --max-time 8 \"" + url + "\" | head -c 300000";
  std::array<char, 4096> buf{};
  std::string html;
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return Response("{\"error\":\"fetch failed\"}", "application/json", "502 Bad Gateway");
  while (fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) html += buf.data();
  pclose(pipe);
  if (html.empty()) return Response("{\"error\":\"empty response\"}", "application/json", "502 Bad Gateway");

  HtmlExtractor extractor;
  auto page = extractor.Extract(html, url);
  if (page.text.empty()) return Response("{\"error\":\"parse failed\"}", "application/json", "422 Unprocessable Entity");

  std::uint32_t max_id = 0;
  for (const auto& d : docs) max_id = std::max(max_id, d.id);
  Document doc;
  doc.id = max_id + 1;
  doc.url = url;
  doc.title = page.title.empty() ? url : page.title;
  doc.body = page.text;
  auto p = url.find("://");
  auto s = (p == std::string::npos) ? 0 : p + 3;
  auto slash = url.find('/', s);
  doc.domain = url.substr(s, slash == std::string::npos ? std::string::npos : slash - s);
  doc.language = "en";
  doc.crawled_at_unix =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  docs.push_back(doc);
  store_->SaveDocuments(docs);
  snapshot_->IncrementalRebuild({doc}, true);
  RebuildIndexFromStore();
  if (os_client_ && os_client_->IsAvailable()) {
    os_client_->UpsertByUrl(doc);
  }

  std::ostringstream json;
  json << "{\"mode\":\"url_preview\",\"backend\":\"" << ((os_client_ && os_client_->IsAvailable()) ? "opensearch" : "local")
       << "\",\"dedup\":false,\"url\":\"" << JsonEscape(url) << "\",\"doc_id\":" << doc.id
       << ",\"title\":\"" << JsonEscape(doc.title) << "\",\"snippet\":\""
       << JsonEscape(doc.body.substr(0, std::min<std::size_t>(200, doc.body.size()))) << "\"}";
  return Response(json.str());
}

std::string HttpServer::HandleSearchJson(const std::string& body) {
  auto qpos = body.find("\"query\"");
  std::string q;
  if (qpos != std::string::npos) {
    auto s = body.find('"', qpos + 7);
    auto e = body.find('"', s + 1);
    if (s != std::string::npos && e != std::string::npos) q = body.substr(s + 1, e - s - 1);
  }
  if (IsUrlQuery(q)) return HandleUrlPreview(q);
  SearchOptions options;
  options.top_k = 10;
  options.explain = true;
  bool hit = false;
  auto start = std::chrono::steady_clock::now();
  std::vector<SearchResult> results;
  std::string backend = "local";
  if (os_client_ && os_client_->IsAvailable()) {
    auto os_hits = os_client_->Search(q, options.top_k);
    for (std::size_t i = 0; i < os_hits.size(); ++i) {
      SearchResult r;
      r.doc_id = static_cast<std::uint32_t>(i + 1);
      r.score = os_hits[i].score;
      r.title = os_hits[i].title;
      r.snippet = os_hits[i].snippet;
      results.push_back(std::move(r));
    }
    backend = "opensearch";
  } else {
    results = service_->Search(q, options, &hit);
  }
  auto end = std::chrono::steady_clock::now();
  auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  std::ostringstream json;
  json << "{\"mode\":\"index_search\",\"backend\":\"" << backend << "\",\"query\":\"" << JsonEscape(q) << "\",\"cache_hit\":"
       << (hit ? "true" : "false")
       << ",\"latency_us\":" << us << ",\"results\":[";
  for (std::size_t i = 0; i < results.size(); ++i) {
    if (i) json << ",";
    json << "{\"id\":" << results[i].doc_id << ",\"url\":\"\","
         << "\"title\":\"" << JsonEscape(results[i].title)
         << "\",\"snippet\":\"" << JsonEscape(results[i].snippet) << "\",\"score\":" << results[i].score
         << "}";
  }
  json << "]}";
  return Response(json.str());
}

std::string HttpServer::HandleSearchHtml(const std::string& path) {
  const std::string q = QueryParam(path, "q");
  std::size_t page = 1;
  std::size_t size = 10;
  auto page_s = QueryParam(path, "page");
  auto size_s = QueryParam(path, "size");
  if (!page_s.empty()) page = std::max<std::size_t>(1, static_cast<std::size_t>(std::stoul(page_s)));
  if (!size_s.empty()) size = std::max<std::size_t>(1, static_cast<std::size_t>(std::stoul(size_s)));

  std::vector<OpenSearchHit> os_hits;
  std::vector<SearchResult> local_hits;
  std::string mode = IsUrlQuery(q) ? "url_preview" : "index_search";
  std::string backend = "local";
  if (IsUrlQuery(q)) {
    auto preview = HandleUrlPreview(q);
    auto body_pos = preview.find("\r\n\r\n");
    const std::string preview_json = body_pos == std::string::npos ? "{}" : preview.substr(body_pos + 4);
    std::string title = ExtractField(preview_json, "title");
    std::string snippet = ExtractField(preview_json, "snippet");
    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset='utf-8'><title>URL Preview</title>";
    html << "<link rel='stylesheet' href='/styles.css'></head><body><main class='container'>";
    html << "<h1>Google Lite</h1><div id='meta'>mode: url_preview</div>";
    html << "<div class='card'><div class='url'>" << JsonEscape(q) << "</div>";
    html << "<a class='title' href='" << JsonEscape(q) << "' target='_blank' rel='noopener noreferrer'>"
         << JsonEscape(title.empty() ? q : title) << "</a>";
    html << "<div class='snippet'>" << JsonEscape(snippet) << "</div></div>";
    html << "</main></body></html>";
    return Response(html.str(), "text/html; charset=utf-8");
  }
  if (os_client_ && os_client_->IsAvailable()) {
    backend = "opensearch";
    os_hits = os_client_->Search(q, page * size);
  } else {
    SearchOptions options;
    options.top_k = page * size;
    bool cache_hit = false;
    local_hits = service_->Search(q, options, &cache_hit);
  }

  std::ostringstream html;
  html << "<!doctype html><html><head><meta charset='utf-8'><title>Google Lite Results</title>";
  html << "<link rel='stylesheet' href='/styles.css'></head><body><main class='container'>";
  html << "<h1>Google Lite</h1>";
  html << "<form action='/search' method='get' class='searchbar'>";
  html << "<input name='q' value='" << JsonEscape(q) << "' />";
  html << "<input type='hidden' name='page' value='1'/>";
  html << "<input type='hidden' name='size' value='" << size << "'/>";
  html << "<button type='submit'>Search</button></form>";
  html << "<div id='meta'>mode: " << mode << " | backend: " << backend << " | page: " << page << "</div>";
  html << "<section id='results'>";
  std::size_t start = (page - 1) * size;
  std::size_t end = page * size;
  if (backend == "opensearch") {
    for (std::size_t i = start; i < os_hits.size() && i < end; ++i) {
      html << "<div class='card'><div class='url'>" << JsonEscape(os_hits[i].url) << "</div>";
      html << "<a class='title' href='" << JsonEscape(os_hits[i].url) << "' target='_blank' rel='noopener noreferrer'>"
           << JsonEscape(os_hits[i].title) << "</a>";
      html << "<div class='snippet'>" << JsonEscape(os_hits[i].snippet) << "</div></div>";
    }
  } else {
    for (std::size_t i = start; i < local_hits.size() && i < end; ++i) {
      html << "<div class='card'><div class='url'>doc " << local_hits[i].doc_id << "</div>";
      html << "<div class='title'>" << JsonEscape(local_hits[i].title) << "</div>";
      html << "<div class='snippet'>" << JsonEscape(local_hits[i].snippet) << "</div></div>";
    }
  }
  html << "</section>";
  html << "<div style='margin-top:16px;display:flex;gap:12px;'>";
  if (page > 1) {
    html << "<a href='/search?q=" << JsonEscape(q) << "&page=" << (page - 1) << "&size=" << size << "'>Prev</a>";
  }
  html << "<a href='/search?q=" << JsonEscape(q) << "&page=" << (page + 1) << "&size=" << size << "'>Next</a>";
  html << "</div>";
  html << "</main></body></html>";
  return Response(html.str(), "text/html; charset=utf-8");
}

std::string HttpServer::HandleSuggest(const std::string& path) {
  auto pos = path.find("q=");
  std::string q = pos == std::string::npos ? "" : UrlDecode(path.substr(pos + 2));
  if (IsUrlQuery(q)) return Response("{\"suggestions\":[]}");
  std::vector<std::string> s;
  if (os_client_ && os_client_->IsAvailable()) s = os_client_->Suggest(q, 6);
  else s = service_->LiveSuggestions(q);
  std::ostringstream json;
  json << "[\"" << JsonEscape(q) << "\",[";
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (i) json << ",";
    json << "\"" << JsonEscape(s[i]) << "\"";
  }
  json << "],[],[]]";
  return Response(json.str());
}

std::string HttpServer::HandleDoc(const std::string& path) {
  auto pos = path.find("id=");
  if (pos == std::string::npos) return Response("{\"error\":\"missing id\"}", "application/json", "400 Bad Request");
  std::string id_s = path.substr(pos + 3);
  if (os_client_ && os_client_->IsAvailable()) {
    OpenSearchHit h;
    if (os_client_->GetById(id_s, &h)) {
      std::ostringstream j;
      j << "{\"id\":\"" << JsonEscape(h.id) << "\",\"url\":\"" << JsonEscape(h.url)
        << "\",\"title\":\"" << JsonEscape(h.title) << "\",\"body\":\"" << JsonEscape(h.snippet) << "\"}";
      return Response(j.str());
    }
  }
  std::uint32_t id = static_cast<std::uint32_t>(std::stoul(id_s));
  const auto* d = index_->GetDocument(id);
  if (!d) return Response("{\"error\":\"not found\"}", "application/json", "404 Not Found");
  std::ostringstream json;
  json << "{\"id\":" << d->id << ",\"url\":\"" << JsonEscape(d->url) << "\",\"title\":\"" << JsonEscape(d->title)
       << "\",\"body\":\"" << JsonEscape(d->body) << "\"}";
  return Response(json.str());
}

std::string HttpServer::HandleHealth() {
  std::ostringstream json;
  json << "{\"status\":\"ok\",\"docs\":" << index_->DocCount() << ",\"requests\":" << request_count_.load()
       << ",\"opensearch\":" << ((os_client_ && os_client_->IsAvailable()) ? "true" : "false") << "}";
  return Response(json.str());
}

std::string HttpServer::HandleRequest(const std::string& req) {
  ++request_count_;
  std::istringstream iss(req);
  std::string method, path, ver;
  iss >> method >> path >> ver;
  if (method == "GET" && path == "/") {
    auto html = ReadFileText("web/index.html");
    return Response(html.empty() ? "<h1>Missing web/index.html</h1>" : html, "text/html; charset=utf-8");
  }
  if (method == "GET" && path == "/app.js") {
    return Response(ReadFileText("web/app.js"), "application/javascript");
  }
  if (method == "GET" && path == "/styles.css") {
    return Response(ReadFileText("web/styles.css"), "text/css");
  }
  if (method == "GET" && path.rfind("/search", 0) == 0) return HandleSearchHtml(path);
  if (method == "GET" && path == "/opensearch.xml") return HandleOpenSearchXml();
  if (method == "GET" && path.rfind("/suggest", 0) == 0) return HandleSuggest(path);
  if (method == "GET" && path.rfind("/doc", 0) == 0) return HandleDoc(path);
  if (method == "GET" && path == "/health") return HandleHealth();
  if (method == "POST" && path == "/search") {
    auto sep = req.find("\r\n\r\n");
    std::string body = sep == std::string::npos ? "" : req.substr(sep + 4);
    return HandleSearchJson(body);
  }
  return Response("{\"error\":\"not found\"}", "application/json", "404 Not Found");
}

bool HttpServer::Start(std::uint16_t port) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) return false;
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) return false;
  if (listen(server_fd, 32) < 0) return false;

  while (true) {
    int client = accept(server_fd, nullptr, nullptr);
    if (client < 0) continue;
    char buffer[8192];
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t n = read(client, buffer, sizeof(buffer) - 1);
    if (n > 0) {
      auto res = HandleRequest(std::string(buffer, static_cast<std::size_t>(n)));
      send(client, res.data(), res.size(), 0);
    }
    close(client);
  }
  close(server_fd);
  return true;
}

}  // namespace glite
