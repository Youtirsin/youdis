#pragma once

#include <algorithm>
#include <atomic>
#include <functional>
#include <iterator>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "resp.hpp"
#include "utils.hpp"
#include "youdis/aof.hpp"
#include "youdis/database.hpp"

namespace resp {
class Command {
 public:
  static auto cmds()
      -> std::unordered_map<std::string,
                            std::function<std::unique_ptr<Value>(
                                const std::vector<std::unique_ptr<Value>>&)>>& {
    static std::unordered_map<std::string,
                              std::function<std::unique_ptr<Value>(
                                  const std::vector<std::unique_ptr<Value>>&)>>
        commands;
    static std::atomic<bool> inited(false);
    if (!inited) {
      inited = true;
      commands["PING"] = ping;
      commands["SET"] = set;
      commands["GET"] = get;
      commands["HSET"] = hset;
      commands["HGET"] = hget;
      commands["HGETALL"] = hget_all;
    }
    return commands;
  }

 private:
  static auto ping(const std::vector<std::unique_ptr<Value>>& args)
      -> std::unique_ptr<Value> {
    if (args.empty()) {
      return Value::make_str("PONG");
    }
    return Value::make_str(
        std::string(args[0]->bulk.begin(), args[0]->bulk.end()));
  }

  static auto set(const std::vector<std::unique_ptr<Value>>& args)
      -> std::unique_ptr<Value> {
    if (args.size() < 2) {
      return Value::make_err("ERR wrong number of arguments for 'set' command");
    }
    std::string key(args[0]->bulk.begin(), args[0]->bulk.end());
    std::string value(args[1]->bulk.begin(), args[1]->bulk.end());

    auto set_ = Database::sets();
    {
      std::lock_guard<std::mutex> guard(set_.second);
      set_.first[key] = value;
    }
    return Value::make_str("OK");
  }

  static auto get(const std::vector<std::unique_ptr<Value>>& args)
      -> std::unique_ptr<Value> {
    if (args.size() != 1) {
      return Value::make_err("ERR wrong number of arguments for 'get' command");
    }
    std::string key(args[0]->bulk.begin(), args[0]->bulk.end());
    std::string value;

    auto set_ = Database::sets();
    {
      std::lock_guard<std::mutex> guard(set_.second);
      if (set_.first.find(key) != set_.first.end()) {
        value = set_.first[key];
      }
    }
    return Value::make_bulk(std::vector<char>(value.begin(), value.end()));
  }

  static auto hset(const std::vector<std::unique_ptr<Value>>& args)
      -> std::unique_ptr<Value> {
    if (args.size() < 3) {
      return Value::make_err("ERR wrong number of arguments for 'hset' command");
    }
    std::string m(args[0]->bulk.begin(), args[0]->bulk.end());
    std::string key(args[1]->bulk.begin(), args[1]->bulk.end());
    std::string value(args[2]->bulk.begin(), args[2]->bulk.end());

    auto hset_ = Database::hsets();
    {
      std::lock_guard<std::mutex> guard(hset_.second);
      hset_.first[m][key] = value;
    }
    return Value::make_str("OK");
  }

  static auto hget(const std::vector<std::unique_ptr<Value>>& args)
      -> std::unique_ptr<Value> {
    if (args.size() != 2) {
      return Value::make_err("ERR wrong number of arguments for 'hget' command");
    }
    std::string m(args[0]->bulk.begin(), args[0]->bulk.end());
    std::string key(args[1]->bulk.begin(), args[1]->bulk.end());
    std::string value;

    auto hset_ = Database::hsets();
    {
      std::lock_guard<std::mutex> guard(hset_.second);
      if (hset_.first.find(m) != hset_.first.end()) {
        if (hset_.first[m].find(key) != hset_.first[m].end()) {
          value = hset_.first[m][key];
        }
      }
    }
    return Value::make_bulk(std::vector<char>(value.begin(), value.end()));
  }

  static auto hget_all(const std::vector<std::unique_ptr<Value>>& args)
      -> std::unique_ptr<Value> {
    if (args.size() != 1) {
      return Value::make_err("ERR wrong number of arguments for 'hgetall' command");
    }
    std::string m(args[0]->bulk.begin(), args[0]->bulk.end());
    std::vector<std::unique_ptr<Value>> values;

    auto hset_ = Database::hsets();
    {
      std::lock_guard<std::mutex> guard(hset_.second);
      if (hset_.first.find(m) != hset_.first.end()) {
        for (auto&& e : hset_.first[m]) {
          auto v = Value::make_bulk(std::vector<char>(e.first.begin(), e.first.end()));
          values.push_back(std::move(v));
        }
      }
    }
    return Value::make_array(std::move(values));
  }

};

class Handler {
 public:
  static auto handle(std::unique_ptr<resp::Value>&& request) -> std::vector<char> {
    auto req = Serializer::marshal(*request);
    static Aof aof;
    if (request->type != types::ARRAY) {
      info() << "invalid request type, expected array" << std::endl;
      return {};
    }

    if (request->array.empty()) {
      info() << "invalid request type, expected array length > 0" << std::endl;
      return {};
    }

    std::string cmdStr;
    std::transform(request->array[0]->bulk.begin(), request->array[0]->bulk.end(),
                   std::back_inserter(cmdStr), toupper);

    std::vector<std::unique_ptr<Value>> args;
    for (auto i = request->array.begin() + 1; i != request->array.end(); ++i) {
      args.push_back(std::move(*i));
    }

    auto reply = Command::cmds()[cmdStr](args);
    auto reply_ = Serializer::marshal(*reply);
    aof.save(req);
    return reply_;
  }
};

};  // namespace resp
