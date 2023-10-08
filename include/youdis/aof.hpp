#pragma once

#include <fstream>
#include <functional>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

#include "resp.hpp"

namespace resp {
class Aof {
 public:
  Aof(const std::string& filepath_ = DEFAULT_NAME)
      : filepath(filepath_) {}

  void save(const std::vector<char>& request) {
    std::lock_guard<std::mutex> guard(mtx);
    std::ofstream writer(filepath, std::ios::app);
    writer.write(request.data(), request.size());
  }

  void load(std::function<void(Value& value)>&& f) {
    std::vector<char> buf;
    Parser parser(buf);
    std::lock_guard<std::mutex> guard(mtx);
    std::ifstream reader(filepath);
    char c;
    while (true) {
      buf.clear();
      for (int i = 0; i < BUF_SIZE && reader.get(c); ++i) {
        buf.push_back(c);
      }
      if (buf.empty()) {
        break;
      }

      parser.reset();
      auto t = parser.parse();
      f(*t);
    }
  }

 private:
  static constexpr const char* DEFAULT_NAME = "database.aof";
  static constexpr int BUF_SIZE = 1024;

  std::string filepath;
  std::mutex mtx;
};

class AofParser {
 public:
 private:
};
};  // namespace resp
