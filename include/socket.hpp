#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

class Socket {
 public:
  static constexpr int BUF_SIZE = 1024;

  Socket() : fd(-1) {}

  Socket(int cfd) : fd(cfd) {}

  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  Socket(Socket&& rhs) {
    *this = std::move(rhs);
  }

  Socket& operator=(Socket&& rhs) {
    this->fd = rhs.fd;
    rhs.fd = -1;
    return *this;
  }

  ~Socket() { close(fd); }

  void reuse_port() {
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  }

  auto raw_fd() -> int { return fd; }

  void create(int domain, int type, int protocol = 0) {
    fd = socket(domain, type, protocol);
    if (fd == -1) {
      throw std::runtime_error("failed to create socket fd.");
    }
  }

  void bind_(const char* address, int port) {
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, address, &(serverAddr.sin_addr)) <= 0) {
      throw std::logic_error("invaild address.");
    }

    if (bind(fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
      throw std::runtime_error("failed to bind address.");
    }
  }

  void listen_(int backlog = 10) {
    if (listen(fd, backlog) == -1) {
      throw std::runtime_error("failed to listen.");
    }
  }

  auto accept_(sockaddr* clientAddr = nullptr,
               socklen_t* clientAddrLen = nullptr) -> int {
    int cfd = accept(fd, clientAddr, clientAddrLen);
    if (cfd == -1) {
      throw std::runtime_error("failed to accept client.");
    }
    return cfd;
  }

  void connect_(const char* address, int port) {
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, address, &(serverAddr.sin_addr)) <= 0) {
      throw std::logic_error("invaild address.");
    }

    if (connect(fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
      throw std::runtime_error("failed to connect.");
    }
  }

  auto send_(const std::vector<char>& data, int flags = 0) -> ssize_t {
    ssize_t sent = send(fd, data.data(), data.size(), flags);
    if (sent == -1) {
      throw std::runtime_error("failed to send.");
    }
    return sent;
  }

  auto receive(int flags = 0) -> std::vector<char> {
    std::vector<char> buffer(BUF_SIZE);
    ssize_t bytesRead = recv(fd, buffer.data(), buffer.size(), flags);
    if (bytesRead > 0) {
      buffer.resize(bytesRead);
    }else if (bytesRead < 0) {
      throw std::runtime_error("failed to receive.");
    } else {
      buffer.clear();
    }
    return buffer;
  }

  auto close_() -> bool {
    if (fd != -1) {
      close(fd);
      fd = -1;
      return true;
    }
    return false;
  }

 private:
  int fd;
};

