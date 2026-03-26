#include "suggest/trie.h"

#include <cctype>

namespace glite {

int Trie::Index(char c) {
  if (c >= 'a' && c <= 'z') return c - 'a';
  if (c >= '0' && c <= '9') return 26 + (c - '0');
  return -1;
}

char Trie::Decode(int idx) {
  return idx < 26 ? static_cast<char>('a' + idx) : static_cast<char>('0' + (idx - 26));
}

void Trie::Insert(const std::string& word) {
  Node* cur = root_.get();
  for (char raw : word) {
    const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(raw)));
    int idx = Index(c);
    if (idx < 0) continue;
    if (!cur->next[idx]) cur->next[idx] = std::make_unique<Node>();
    cur = cur->next[idx].get();
  }
  cur->terminal = true;
}

void Trie::Dfs(const Node* node, std::string& cur, std::vector<std::string>& out,
               std::size_t max_results) const {
  if (out.size() >= max_results) return;
  if (node->terminal) out.push_back(cur);
  for (int i = 0; i < 36 && out.size() < max_results; ++i) {
    if (!node->next[i]) continue;
    cur.push_back(Decode(i));
    Dfs(node->next[i].get(), cur, out, max_results);
    cur.pop_back();
  }
}

std::vector<std::string> Trie::Suggest(const std::string& prefix, std::size_t max_results) const {
  const Node* cur = root_.get();
  std::string clean;
  for (char raw : prefix) {
    char c = static_cast<char>(std::tolower(static_cast<unsigned char>(raw)));
    int idx = Index(c);
    if (idx < 0 || !cur->next[idx]) return {};
    clean.push_back(c);
    cur = cur->next[idx].get();
  }
  std::vector<std::string> out;
  std::string work = clean;
  Dfs(cur, work, out, max_results);
  return out;
}

}  // namespace glite
