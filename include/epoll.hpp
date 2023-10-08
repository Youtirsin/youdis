#pragma once

#include <sys/epoll.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <vector>

class Epoll {
 public:
  Epoll() : fd(epoll_create1(0)) {
    if (fd == -1) {
      throw std::runtime_error("failed to create epoll.");
    }
  }

  Epoll(const Epoll&) = delete;
  Epoll& operator=(const Epoll&) = delete;

  Epoll(Epoll&& rhs) {
    *this = std::move(rhs);
  }

  Epoll& operator=(Epoll&& rhs) {
    this->fd = rhs.fd;
    rhs.fd = -1;
    return *this;
  }

  ~Epoll() { close(fd); }

  void add_socket(int socket, uint32_t events) {
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = socket;
    event.events = events;

    if (epoll_ctl(fd, EPOLL_CTL_ADD, socket, &event) == -1) {
      throw std::runtime_error("failed to add socket to epoll");
    }
  }

  void modify_socket(int socket, uint32_t events) {
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = socket;
    event.events = events;

    if (epoll_ctl(fd, EPOLL_CTL_MOD, socket, &event) == -1) {
      throw std::runtime_error("failed to modify socket in epoll");
    }
  }

  void remove_socket(int socket) {
    if (epoll_ctl(fd, EPOLL_CTL_DEL, socket, nullptr) == -1) {
      throw std::runtime_error("failed to remove socket from epoll");
    }
  }

  auto wait(std::vector<epoll_event>& events, int timeoutMs) -> int {
    int numEvents = epoll_wait(fd, events.data(),
                               static_cast<int>(events.size()), timeoutMs);
    if (numEvents == -1) {
      throw std::runtime_error("epoll wait error");
    }
    return numEvents;
  }

 private:
  int fd;
};

