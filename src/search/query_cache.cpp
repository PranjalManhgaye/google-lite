#include "search/query_cache.h"

namespace glite {

bool QueryCache::Get(const std::string& key, std::vector<SearchResult>* value) {
  auto it = index_.find(key);
  if (it == index_.end()) return false;
  lru_.splice(lru_.begin(), lru_, it->second);
  *value = it->second->value;
  return true;
}

void QueryCache::Put(const std::string& key, const std::vector<SearchResult>& value) {
  auto it = index_.find(key);
  if (it != index_.end()) {
    it->second->value = value;
    lru_.splice(lru_.begin(), lru_, it->second);
    return;
  }
  lru_.push_front(Entry{key, value});
  index_[key] = lru_.begin();
  if (lru_.size() > capacity_) {
    auto last = --lru_.end();
    index_.erase(last->key);
    lru_.pop_back();
  }
}

}  // namespace glite
