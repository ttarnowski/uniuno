#pragma once

#include <Future.h>
#include <Schedule.h>
#include <function_traits.h>
#include <queue>

namespace uniuno {

class Executor {
public:
  template <typename I, typename O, typename E = Error, class F,
            typename std::enable_if<
                function_traits<F>::arity == 1 &&
                std::is_same<typename function_traits<F>::template arg<0>::type,
                             E>::value>::type * = nullptr>
  void execute(Future<I, O, E> future, I input, F &&on_error) {
    this->futures.push([future, input, on_error]() mutable {
      auto result = future.poll(input);

      if (result.is_rejected()) {
        on_error(*result.get_error());
      }

      return result.get_state();
    });

    this->schedule();
  }

  template <typename O, typename E = Error, class F,
            typename std::enable_if<
                function_traits<F>::arity == 1 &&
                std::is_same<typename function_traits<F>::template arg<0>::type,
                             E>::value>::type * = nullptr>
  void execute(Future<void, O> future, F &&on_error) {
    this->futures.push([future, on_error]() mutable {
      auto result = future.poll();

      if (result.is_rejected()) {
        on_error(*result.get_error());
      }

      return result.get_state();
    });

    this->schedule();
  }

  template <typename I, typename O, typename E = Error>
  void execute(Future<I, O, E> future, I input) {
    this->execute(future, input, [](E) {});
  }

  template <typename O, typename E = Error>
  void execute(Future<void, O, E> future) {
    this->execute(future, [](E) {});
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

} // namespace uniuno