#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>

class Threadpool {
public:
  Threadpool(int num = 8) : ctx(std::make_shared<Context>()) {
    for (int i = 0; i < num; ++i) {
      std::thread([=]() {
        while (true) {
          std::unique_lock<std::mutex> locker(ctx->mtx);
          if (ctx->closed) {
            break;
          }
          ctx->cond.wait(locker);
          if (!ctx->tasks.empty()) {
            auto task = std::move(ctx->tasks.front());
            ctx->tasks.pop();
            locker.unlock();
            task();
          }
        }
      }).detach();
    }
  }

  Threadpool(const Threadpool &) = delete;
  Threadpool &operator=(const Threadpool &) = delete;
  Threadpool(Threadpool &&) = delete;
  Threadpool &operator=(Threadpool &&) = delete;

  ~Threadpool() { close(); };

  template <class F, class... Args>
  auto add_task(F &&func, Args &&...args)
      -> std::future<decltype(func(args...))> {
    auto f = std::bind(std::forward<F>(func), std::forward<F>(args)...);
    auto p = std::make_shared<std::packaged_task<decltype(f())()>>(f);
    auto task = [p]() { (*p)(); };
    {
      std::lock_guard<std::mutex> guard(ctx->mtx);
      ctx->tasks.push(std::move(task));
    }
    ctx->cond.notify_one();
    return p->get_future();
  }

  void close() {
    {
      std::lock_guard<std::mutex> guard(ctx->mtx);
      ctx->closed = true;
    }
    ctx->cond.notify_all();
  }

private:
  struct Context {
    std::queue<std::function<void()>> tasks;
    std::condition_variable cond;
    std::mutex mtx;
    bool closed;

    Context() : closed(false) {}
  };

  std::shared_ptr<Context> ctx;
};
