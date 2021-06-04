#pragma once

#include <Future.hpp>
#include <queue>

class Executor {
public:
  template <typename I, typename O, typename E = Error>
  void execute(Future<I, O, E> future, I input,
               std::function<void(E)> on_error = nullptr) {
    this->futures.push([future, input, on_error]() mutable {
      auto result = future.poll(input);

      if (on_error != nullptr && result.is_rejected()) {
        on_error(*result.get_error());
      }

      return result.get_state();
    });

    this->schedule();
  }

  template <typename O, typename E = Error>
  void execute(Future<void, O> future,
               std::function<void(E)> on_error = nullptr) {
    this->futures.push([future, on_error]() mutable {
      auto result = future.poll();

      if (on_error != nullptr && result.is_rejected()) {
        on_error(*result.get_error());
      }

      return result.get_state();
    });

    this->schedule();
  }

private:
  void poll() {
    if (this->futures.empty()) {
      return;
    }

    auto state = this->futures.front()();

    if (state != AsyncState::Pending) {
      this->futures.pop();
    }

    this->schedule();
  }

  void schedule() {
    schedule_function([this]() {
      if (this == nullptr) {
        return;
      }
      this->poll();
    });
  }

  std::queue<std::function<AsyncState(void)>> futures;
  int index = 0;
};
