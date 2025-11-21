module;

export module threadpool;

import std;

export template <typename Signature>
using MoveOnlyFunction = std::move_only_function<Signature>;

export class ThreadPool {
private:
  std::deque<MoveOnlyFunction<void()>> tasks_{};
  std::vector<std::thread> threads_{};
  std::mutex lock_{};
  std::condition_variable cv_{};
  bool stopped_ = false;

public:
  explicit ThreadPool(std::size_t threads) {
    for (std::size_t i = 0; i < threads; ++i) {
      threads_.emplace_back([this]() {
        while (true) {
          auto task = MoveOnlyFunction<void()>{}; // benchmark inside or outside
          {
            auto ulock = std::unique_lock<std::mutex>{lock_};
            cv_.wait(ulock, [this]() {
              return this->stopped_ || !this->tasks_.empty();
            });

            if (stopped_) {
              return;
            }

            task = std::move(tasks_.front());
            tasks_.pop_front();
          }
          task();
        }
      });
    }
  }
  ~ThreadPool() {
    if (!stopped_) {
      Stop();
    }
  }
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  auto operator=(ThreadPool) -> ThreadPool & = delete;

  std::size_t NumberOfWorkitems() {
    const auto guard = std::lock_guard<std::mutex>{lock_};
    return tasks_.size();
  }

  std::size_t NumberOfThreads() const {
    return threads_.size();
  }

  bool PoolIsBusy() {
    const auto guard = std::lock_guard<std::mutex>{lock_};
    return !tasks_.empty();
  }

  auto Stop() -> void {
    stopped_ = true;
    cv_.notify_all();
    for (auto &thread : threads_) {
      thread.join();
    }
  }

  template <typename Callable, typename... Args>
  auto Enqueue(Callable &&func, Args &&...args) {
    using Return_Type = std::invoke_result_t<Callable, Args...>;

    auto promise = std::promise<Return_Type>{};
    auto future = promise.get_future();
    {
      const auto guard = std::lock_guard<std::mutex>{lock_};
      tasks_.emplace_back(
          [promise = std::move(promise),
           task = std::bind(std::forward<Callable>(func),
                            std::forward<Args>(args)...)]() mutable {
            if constexpr (std::is_void_v<Return_Type>) {
              std::invoke(task);
              promise.set_value();
            } else {
              promise.set_value(std::invoke(task));
            }
          });
    }
    cv_.notify_one();
    return std::move(future);
  }
};
