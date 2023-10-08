#pragma once

#include "socket.hpp"
#include "youdis/resp.hpp"

namespace resp {

class SocketClose : public std::exception {
 public:
  SocketClose(const char* msg) : message(msg) {}

  auto what() const noexcept -> const char* { return message.c_str(); }

 private:
  std::string message;
};

class SocketReadable : public Readable {
 public:
  SocketReadable(Socket& socket_)
      : socket(socket_), buf(Socket::BUF_SIZE), it(buf.end()) {}

  auto readline(std::vector<char>& line) -> bool {
    line.clear();
    fetch();

    for (;it != buf.end(); ++it) {
      if (*it == '\n' && line.back() == '\r') {
        line.pop_back();
        ++it;
        return true;
      }
      line.push_back(*it);
    }
    return false;
  }

  auto readline() -> bool {
    fetch();

    char pre = '\0';
    for (;it != buf.end(); ++it) {
      if (*it == '\n' && pre == '\r') {
        ++it;
        return true;
      }
      pre = *it;
    }
    return false;
  }

  auto read_byte(char& c) -> bool {
    fetch();
    if (it == buf.end()) {
      return false;
    }
    c = *it;
    ++it;
    return true;
  }

  auto read_n(std::vector<char>& buffer, int n) -> bool {
    buffer.clear();
    fetch();

    char byte = '\0';
    for (int i = 0; i < n; ++i) {
      if (!read_byte(byte)) {
        return false;
      }
      buffer.push_back(byte);
    }
    return true;
  }

 private:
  void fetch() {
    if (it == buf.end()) {
      buf = socket.receive();
      it = buf.begin();
    }
    if (it == buf.end()) {
      throw SocketClose("socket disconnected.");
    }
  }

  Socket& socket;

  std::vector<char> buf;
  std::vector<char>::const_iterator it;
};
};  // namespace resp
