#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace resp {
class Database {
 public:
  static auto sets()
      -> std::pair<std::unordered_map<std::string, std::string>&, std::mutex&> {
    static std::unordered_map<std::string, std::string> m;
    static std::mutex mtx;
    return {m, mtx};
  }

  static auto hsets() -> std::pair<
      std::unordered_map<std::string,
                         std::unordered_map<std::string, std::string>>&,
      std::mutex&> {
    static std::unordered_map<std::string,
                              std::unordered_map<std::string, std::string>>
        m;
    static std::mutex mtx;
    return {m, mtx};
  }
};
};  // namespace resp
