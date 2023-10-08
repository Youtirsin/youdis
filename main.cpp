#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include "epoll.hpp"
#include "socket.hpp"
#include "utils.hpp"
#include "youdis/aof.hpp"
#include "youdis/handle.hpp"
#include "youdis/resp.hpp"
#include "youdis/socket_readable.hpp"

int main(int argc, char** argv) {
  try {
    Socket server;
    server.create(AF_INET, SOCK_STREAM);
    server.reuse_port();
    server.bind_("127.0.0.1", 6379);
    server.listen_();
    info() << "listening at 127.0.0.1:6379..." << std::endl;

    Epoll epoll;
    epoll.add_socket(server.raw_fd(), EPOLLIN);

    std::vector<epoll_event> events(16);
    std::unordered_map<int, std::pair<Socket, std::mutex>> clients;

    while (true) {
      int numEvents = epoll.wait(events, -1);
      for (int i = 0; i < numEvents; ++i) {
        int fd = events[i].data.fd;

        // connection
        if (fd == server.raw_fd()) {
          int cfd = server.accept_();
          clients[cfd];
          clients[cfd].first = Socket(cfd);
          epoll.add_socket(cfd, EPOLLIN);
          continue;
        }

        try {
          std::unique_ptr<resp::Value> request;
          {
            std::lock_guard<std::mutex> guard(clients[fd].second);
            resp::SocketReadable reader(clients[fd].first);
            request = resp::GeneralParser(reader).parse();
          }
          auto response = resp::Handler::handle(std::move(request));
          {
            std::lock_guard<std::mutex> guard(clients[fd].second);
            clients[fd].first.send_(response);
          }
        } catch (const resp::SocketClose& e) {
          epoll.remove_socket(fd);
          clients.erase(fd);
        } catch (const std::exception& e) {
          epoll.remove_socket(fd);
          clients.erase(fd);
          error() << e.what() << std::endl;
        }
      }
    }

  } catch (const std::exception& e) {
    error() << e.what() << std::endl;
    return 1;
  }

  return 0;
}
