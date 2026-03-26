#include "io/wiki_loader.h"

#include <fstream>
#include <sstream>

namespace glite {

std::vector<Document> WikiLoader::LoadTsv(const std::string& path, std::size_t max_docs) const {
  std::ifstream in(path);
  std::vector<Document> docs;
  if (!in) {
    return docs;
  }

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    std::stringstream ss(line);
    std::string id_s, title, body;
    if (!std::getline(ss, id_s, '\t') || !std::getline(ss, title, '\t') || !std::getline(ss, body)) {
      continue;
    }
    try {
      Document doc;
      doc.id = static_cast<std::uint32_t>(std::stoul(id_s));
      doc.url = "local://doc/" + id_s;
      doc.title = std::move(title);
      doc.body = std::move(body);
      doc.domain = "local";
      docs.push_back(std::move(doc));
      if (max_docs > 0 && docs.size() >= max_docs) {
        break;
      }
    } catch (...) {
      continue;
    }
  }
  return docs;
}

}  // namespace glite
