#pragma once

#include <Error.h>

enum AsyncState { Pending = 0, Resolved, Rejected };

template <typename T, typename E = Error> class AsyncResult {
public:
  static AsyncResult<T, E> pending() { return AsyncResult<T, E>(); }

  static AsyncResult<T, E> reject(E err) { return AsyncResult<T, E>(err); }

  static AsyncResult<T, E> resolve(T input) { return AsyncResult<T, E>(input); }

  AsyncResult() { this->state = AsyncState::Pending; }
  AsyncResult(T input) {
    this->value = input;
    this->state = AsyncState::Resolved;
  }

  AsyncResult(E e) {
    this->err = e;
    this->state = AsyncState::Rejected;
  }

  bool is_pending() { return this->state == AsyncState::Pending; }

  bool is_resolved() { return this->state == AsyncState::Resolved; }

  bool is_rejected() { return this->state == AsyncState::Rejected; }

  T *get_value() { return this->is_resolved() ? &value : nullptr; }
  E *get_error() { return this->is_rejected() ? &err : nullptr; }
  AsyncState get_state() { return this->state; }

private:
  AsyncState state;
  T value;
  E err;
};

template <typename E> class AsyncResult<void, E> {
public:
  static AsyncResult<void, E> pending() {
    return AsyncResult<void, E>(AsyncState::Pending);
  }

  static AsyncResult<void, E> reject(E err) {
    return AsyncResult<void, E>(err);
  }

  static AsyncResult<void, E> resolve() {
    return AsyncResult<void, E>(AsyncState::Resolved);
  }

  AsyncResult() { this->state = AsyncState::Pending; }

  AsyncResult(AsyncState state) { this->state = state; }

  AsyncResult(E e) {
    this->err = e;
    this->state = AsyncState::Rejected;
  }

  bool is_pending() { return this->state == AsyncState::Pending; }

  bool is_resolved() { return this->state == AsyncState::Resolved; }

  bool is_rejected() { return this->state == AsyncState::Rejected; }

  E *get_error() { return this->is_rejected() ? &err : nullptr; }
  AsyncState get_state() { return this->state; }

private:
  AsyncState state;
  E err;
};