#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace resp {
struct types {
  static constexpr char STRING = '+';
  static constexpr char ERROR = '-';
  static constexpr char NIL = 'e';
  static constexpr char INTEGER = ':';
  static constexpr char BULK = '$';
  static constexpr char ARRAY = '*';
};

// using classes and inherit
struct Value {
  char type;
  int num;
  std::string str;
  std::vector<char> bulk;
  std::vector<std::unique_ptr<Value>> array;

  static auto make_str(const std::string& s = "") -> std::unique_ptr<Value> {
    std::unique_ptr<Value> val(new Value);
    val->type = types::STRING;
    val->str = s;
    return val;
  }

  static auto make_err(const std::string& e = "") -> std::unique_ptr<Value> {
    std::unique_ptr<Value> val(new Value);
    val->type = types::ERROR;
    val->str = e;
    return val;
  }

  static auto make_bulk(const std::vector<char>& b = {})
      -> std::unique_ptr<Value> {
    std::unique_ptr<Value> val(new Value);
    val->type = types::BULK;
    val->bulk = b;
    return val;
  }

  static auto make_array(std::vector<std::unique_ptr<Value>>&& a = {})
      -> std::unique_ptr<Value> {
    std::unique_ptr<Value> val(new Value);
    val->type = types::ARRAY;
    val->array = std::move(a);
    return val;
  }
};

class Readable {
 public:
  virtual auto readline(std::vector<char>&) -> bool = 0;
  virtual auto readline() -> bool = 0;
  virtual auto read_byte(char&) -> bool = 0;
  virtual auto read_n(std::vector<char>&, int) -> bool = 0;
};

class GeneralParser {
 public:
  GeneralParser(Readable& reader_) : reader(reader_) {}

  auto parse() -> std::unique_ptr<Value> {
    char type = '\0';
    if (!reader.read_byte(type)) {
      throw std::runtime_error("failed to parse resp value.");
    }
    if (type == types::ARRAY) {
      return read_array();
    }
    if (type == types::BULK) {
      return read_bulk();
    }
    throw std::runtime_error("unknown resp value type.");
  }

 private:
  auto read_num() -> int {
    std::vector<char> line;
    if (!reader.readline(line)) {
      throw std::runtime_error("failed to read number.");
    }
    return stoi(std::string(line.begin(), line.end()));
  }

  auto read_array() -> std::unique_ptr<Value> {
    auto val = Value::make_array();

    int len = read_num();
    val->array.resize(len);

    for (int i = 0; i < len; ++i) {
      val->array[i] = std::unique_ptr<Value>(parse());
    }
    return val;
  }

  auto read_bulk() -> std::unique_ptr<Value> {
    auto val = Value::make_bulk();

    int len = read_num();
    val->bulk.resize(len);
    if (!reader.read_n(val->bulk, len)) {
      throw std::runtime_error("failed to read bulk.");
    }
    reader.readline();
    return val;
  }
  
  Readable& reader;
};

class Parser {
 public:
  Parser(const std::vector<char>& buffer) : buf(buffer), it(buf.begin()) {}

  auto parse() -> std::unique_ptr<Value> {
    char type;
    if (!read_byte(type)) {
      throw std::runtime_error("failed to parse resp value.");
    }
    if (type == types::ARRAY) {
      return read_array();
    }
    if (type == types::BULK) {
      return read_bulk();
    }
    throw std::runtime_error("unknown resp value type.");
  }

  void reset() { it = buf.begin(); }

 private:
  auto read_num() -> int {
    std::vector<char> line;
    if (!readline(line)) {
      throw std::runtime_error("failed to read number.");
    }
    return stoi(std::string(line.begin(), line.end()));
  }

  auto read_array() -> std::unique_ptr<Value> {
    auto val = Value::make_array();

    int len = read_num();
    val->array.resize(len);

    for (int i = 0; i < len; ++i) {
      val->array[i] = std::unique_ptr<Value>(parse());
    }
    return val;
  }

  auto read_bulk() -> std::unique_ptr<Value> {
    auto val = Value::make_bulk();

    int len = read_num();
    val->bulk.resize(len);
    if (!read_n(val->bulk, len)) {
      throw std::runtime_error("failed to read bulk.");
    }
    readline();
    return val;
  }

  auto read_byte(char& byte) -> bool {
    if (it == buf.end()) {
      return false;
    }
    byte = *it;
    it++;
    return true;
  }

  auto readline() -> bool {
    char pre = '\0';
    for (; it != buf.end(); ++it) {
      if (*it == '\n' && pre == '\r') {
        ++it;
        return true;
      }
      pre = *it;
    }
    return false;
  }

  auto readline(std::vector<char>& line) -> bool {
    line.clear();
    for (; it != buf.end(); ++it) {
      if (*it == '\n' && line.back() == '\r') {
        line.pop_back();
        ++it;
        return true;
      }
      line.push_back(*it);
    }
    return false;
  };

  auto read_n(std::vector<char>& line, int n) -> bool {
    line.clear();
    for (int i = 0; i < n; ++i) {
      if (it == buf.end()) {
        return false;
      }
      line.push_back(*it);
      ++it;
    }
    return true;
  }

 private:
  const std::vector<char> buf;
  std::vector<char>::const_iterator it;
};

class Serializer {
 public:
  static auto marshal(const Value& val) -> std::vector<char> {
    if (val.type == types::ARRAY) {
      return marshal_array(val);
    }
    if (val.type == types::BULK) {
      return marshal_bulk(val);
    }
    if (val.type == types::STRING) {
      return marshal_string(val);
    }
    if (val.type == types::ERROR) {
      return marshal_error(val);
    }
    if (val.type == types::NIL) {
      return marshal_null(val);
    }
    return {};
  }

 private:
  static auto marshal_array(const Value& val) -> std::vector<char> {
    std::vector<char> res = {val.type};
    std::string len(std::to_string(val.array.size()));
    res.insert(res.end(), len.begin(), len.end());
    res.push_back('\r');
    res.push_back('\n');
    for (auto& p : val.array) {
      auto v = marshal(*p);
      res.insert(res.end(), v.begin(), v.end());
    }
    return res;
  }

  static auto marshal_string(const Value& val) -> std::vector<char> {
    std::vector<char> res = {val.type};
    res.insert(res.end(), val.str.begin(), val.str.end());
    res.push_back('\r');
    res.push_back('\n');
    return res;
  }

  static auto marshal_bulk(const Value& val) -> std::vector<char> {
    std::vector<char> res = {val.type};
    std::string len(std::to_string(val.bulk.size()));
    res.insert(res.end(), len.begin(), len.end());
    res.push_back('\r');
    res.push_back('\n');
    res.insert(res.end(), val.bulk.begin(), val.bulk.end());
    res.push_back('\r');
    res.push_back('\n');
    return res;
  }

  static auto marshal_error(const Value& val) -> std::vector<char> {
    std::vector<char> res = {val.type};
    res.insert(res.end(), val.str.begin(), val.str.end());
    res.push_back('\r');
    res.push_back('\n');
    return res;
  }

  static auto marshal_null(const Value& val) -> std::vector<char> {
    return {'$', '-', '1', '\r', '\n'};
  }
};
};  // namespace resp
