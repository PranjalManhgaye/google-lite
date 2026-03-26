#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "search/search_types.h"

namespace glite {

class QueryCache {
 public:
  explicit QueryCache(std::size_t capacity = 128) : capacity_(capacity) {}
  bool Get(const std::string& key, std::vector<SearchResult>* value);
  void Put(const std::string& key, const std::vector<SearchResult>& value);

 private:
  struct Entry {
    std::string key;
    std::vector<SearchResult> value;
  };
  std::size_t capacity_;
  std::list<Entry> lru_;
  std::unordered_map<std::string, std::list<Entry>::iterator> index_;
};

}  // namespace glite
