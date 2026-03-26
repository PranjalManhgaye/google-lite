#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace glite {

struct Document {
  std::uint32_t id{};
  std::string url;
  std::string title;
  std::string body;
  std::string domain;
  std::string language{"en"};
  std::int64_t crawled_at_unix{0};

  Document() = default;
  Document(std::uint32_t in_id, std::string in_title, std::string in_body)
      : id(in_id), url("local://doc/" + std::to_string(in_id)), title(std::move(in_title)),
        body(std::move(in_body)), domain("local"), language("en"), crawled_at_unix(0) {}
};

}  // namespace glite
