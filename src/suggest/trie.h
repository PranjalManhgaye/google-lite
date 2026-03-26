#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace glite {

class Trie {
 public:
  void Insert(const std::string& word);
  std::vector<std::string> Suggest(const std::string& prefix, std::size_t max_results = 5) const;

 private:
  struct Node {
    std::array<std::unique_ptr<Node>, 36> next{};
    bool terminal{false};
  };

  std::unique_ptr<Node> root_ = std::make_unique<Node>();
  static int Index(char c);
  static char Decode(int idx);
  void Dfs(const Node* node, std::string& cur, std::vector<std::string>& out,
           std::size_t max_results) const;
};

}  // namespace glite
