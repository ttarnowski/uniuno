#pragma once

#include <AsyncResult.h>
#include <function_traits.h>
#include <functional>

using namespace utils;

template <typename I, typename O, typename E = Error> class Future {
public:
  Future(std::function<AsyncResult<O, E>(I)> callback) {
    this->poll_callback = callback;
    this->result = AsyncResult<O, E>::pending();
  }

  static Future<I, O, E> resolve(O output) {
    return Future<I, O, E>(
        [output](I) { return AsyncResult<O, E>::resolve(output); });
  }

  static Future<I, O, E> reject(E error) {
    return Future<I, O, E>(
        [error](I) { return AsyncResult<O, E>::reject(error); });
  }

  AsyncResult<O, E> poll(I input) {
    if (this->result.is_pending()) {
      this->result = this->poll_callback(input);
    }
    return this->result;
  }

  template <typename N, typename G = Error>
  Future<I, N, G> and_then(Future<O, N, G> next_future) {
    auto this_future = *this;

    return Future<I, N, G>([this_future,
                            next_future](I input) mutable -> AsyncResult<N, G> {
      if (this_future.result.is_pending()) {
        this_future.poll(input);
      }

      if (this_future.result.is_pending()) {
        return AsyncResult<N, G>::pending();
      }

      if (this_future.result.is_rejected()) {
        return AsyncResult<N, G>::reject(G(*this_future.result.get_error()));
      }

      return next_future.poll(*this_future.result.get_value());
    });
  }

  template <typename F>
  auto and_then(F &&fn) -> Future<I, decltype(fn(std::declval<O>()))> {
    return this->and_then<decltype(fn(std::declval<O>()))>(
        Future<O, decltype(fn(std::declval<O>()))>([fn](O input) {
          return AsyncResult<decltype(fn(std::declval<O>()))>::resolve(
              fn(input));
        }));
  }

  AsyncResult<O, E> result;

private:
  std::function<AsyncResult<O, E>(I)> poll_callback;
};

template <typename I, typename E> class Future<I, void, E> {
public:
  Future(std::function<AsyncResult<void, E>(I)> callback) {
    this->poll_callback = callback;
    this->result = AsyncResult<void, E>::pending();
  }

  static Future<I, void, E> resolve() {
    return Future<I, void, E>([]() { return AsyncResult<void, E>::resolve(); });
  }

  static Future<I, void, E> reject(E error) {
    return Future<I, void, E>(
        [error](I) { return AsyncResult<void, E>::reject(error); });
  }

  AsyncResult<void, E> poll(I input) {
    if (this->result.is_pending()) {
      this->result = this->poll_callback(input);
    }
    return this->result;
  }

  template <typename N, typename G = Error>
  Future<I, N, G> and_then(Future<void, N, G> next_future) {
    auto this_future = *this;

    return Future<I, N, G>([this_future,
                            next_future](I input) mutable -> AsyncResult<N, G> {
      if (this_future.result.is_pending()) {
        this_future.poll(input);
      }

      if (this_future.result.is_pending()) {
        return AsyncResult<N, G>::pending();
      }

      if (this_future.result.is_rejected()) {
        return AsyncResult<N, G>::reject(G(*this_future.result.get_error()));
      }

      return next_future.poll();
    });
  }

  template <typename F> auto and_then(F &&fn) -> Future<I, decltype(fn())> {
    return this->and_then<decltype(fn())>(Future<void, decltype(fn())>(
        [fn]() { return AsyncResult<decltype(fn())>::resolve(fn()); }));
  }

  AsyncResult<void, E> result;

private:
  std::function<AsyncResult<void, E>(I)> poll_callback;
};

template <typename O, typename E> class Future<void, O, E> {
public:
  Future(std::function<AsyncResult<O, E>()> callback) {
    this->poll_callback = callback;
    this->result = AsyncResult<O, E>::pending();
  }

  static Future<void, O, E> resolve(O output) {
    return Future<void, O, E>(
        [output]() { return AsyncResult<O, E>::resolve(output); });
  }

  static Future<void, O, E> reject(E error) {
    return Future<void, O, E>(
        [error]() { return AsyncResult<O, E>::reject(error); });
  }

  AsyncResult<O, E> poll() {
    if (this->result.is_pending()) {
      this->result = this->poll_callback();
    }
    return this->result;
  }

  template <typename N, typename G = Error>
  Future<void, N, G> and_then(Future<O, N, G> next_future) {
    auto this_future = *this;

    return Future<void, N, G>([this_future,
                               next_future]() mutable -> AsyncResult<N, G> {
      if (this_future.result.is_pending()) {
        this_future.poll();
      }

      if (this_future.result.is_pending()) {
        return AsyncResult<N, G>::pending();
      }

      if (this_future.result.is_rejected()) {
        return AsyncResult<N, G>::reject(G(*this_future.result.get_error()));
      }

      return next_future.poll(*this_future.result.get_value());
    });
  }

  template <class F>
  auto and_then(F &&fn) -> Future<void, decltype(fn(std::declval<O>()))> {
    return this->into_future(fn);
  }

  AsyncResult<O, E> result;

private:
  template <class F,
            typename std::enable_if<
                std::is_same<typename function_traits<F>::template arg<0>::type,
                             O>::value &&
                !std::is_same<typename function_traits<F>::result_type,
                              void>::value>::type * = nullptr>
  auto into_future(F &&fn) -> Future<void, decltype(fn(std::declval<O>()))> {
    return this->and_then<decltype(fn(std::declval<O>()))>(
        Future<O, decltype(fn(std::declval<O>()))>([fn](O input) {
          return AsyncResult<decltype(fn(std::declval<O>()))>::resolve(
              fn(input));
        }));
  }

  template <class F,
            typename std::enable_if<
                std::is_same<typename function_traits<F>::template arg<0>::type,
                             O>::value &&
                std::is_same<typename function_traits<F>::result_type,
                             void>::value>::type * = nullptr>
  auto into_future(F &&fn) -> Future<void, decltype(fn(std::declval<O>()))> {
    return this->and_then<decltype(fn(std::declval<O>()))>(
        Future<O, decltype(fn(std::declval<O>()))>([fn](O input) {
          fn(input);
          return AsyncResult<void>::resolve();
        }));
  }

  std::function<AsyncResult<O, E>()> poll_callback;
};

template <typename E> class Future<void, void, E> {
public:
  Future(std::function<AsyncResult<void, E>()> callback) {
    this->poll_callback = callback;
    this->result = AsyncResult<void, E>::pending();
  }

  static Future<void, void, E> resolve() {
    return Future<void, void, E>(
        []() { return AsyncResult<void, E>::resolve(); });
  }

  static Future<void, void, E> reject(E error) {
    return Future<void, void, E>(
        [error]() { return AsyncResult<void, E>::reject(error); });
  }

  AsyncResult<void, E> poll() {
    if (this->result.is_pending()) {
      this->result = this->poll_callback();
    }

    return this->result;
  }

  template <typename N, typename G = Error>
  Future<void, N, G> and_then(Future<void, N, G> next_future) {
    auto this_future = *this;

    return Future<void, N, G>([this_future,
                               next_future]() mutable -> AsyncResult<N, G> {
      if (this_future.result.is_pending()) {
        this_future.poll();
      }

      if (this_future.result.is_pending()) {
        return AsyncResult<N, G>::pending();
      }

      if (this_future.result.is_rejected()) {
        return AsyncResult<N, G>::reject(G(*this_future.result.get_error()));
      }

      return next_future.poll();
    });
  }

  template <class F> auto and_then(F &&fn) -> Future<void, decltype(fn())> {
    return this->into_future(fn);
  }

  AsyncResult<void, E> result;

private:
  template <class F, typename std::enable_if<
                         !std::is_same<typename function_traits<F>::result_type,
                                       void>::value>::type * = nullptr>
  auto into_future(F &&fn) -> Future<void, decltype(fn())> {
    return this->and_then<decltype(fn())>(Future<void, decltype(fn())>(
        [fn]() { return AsyncResult<decltype(fn())>::resolve(fn()); }));
  }

  template <class F, typename std::enable_if<
                         std::is_same<typename function_traits<F>::result_type,
                                      void>::value>::type * = nullptr>
  auto into_future(F &&fn) -> Future<void, decltype(fn())> {
    return this->and_then<decltype(fn())>(Future<void, decltype(fn())>([fn]() {
      fn();
      return AsyncResult<void>::resolve();
    }));
  }

  std::function<AsyncResult<void, E>()> poll_callback;
};

template <typename T> struct async_result_trait {};

template <typename ResultType>
struct async_result_trait<AsyncResult<ResultType>> {
  typedef ResultType result_type;
};

template <class F, typename std::enable_if<function_traits<F>::arity == 1>::type
                       * = nullptr>
auto create_future(F &&poll_fn)
    -> Future<typename function_traits<F>::template arg<0>::type,
              typename async_result_trait<
                  typename function_traits<F>::result_type>::result_type> {
  return Future<typename function_traits<F>::template arg<0>::type,
                typename async_result_trait<
                    typename function_traits<F>::result_type>::result_type>(
      poll_fn);
}

template <class F, typename std::enable_if<function_traits<F>::arity == 0>::type
                       * = nullptr>
auto create_future(F &&poll_fn)
    -> Future<void, typename async_result_trait<typename function_traits<
                        F>::result_type>::result_type> {
  return Future<void, typename async_result_trait<typename function_traits<
                          F>::result_type>::result_type>(poll_fn);
}
