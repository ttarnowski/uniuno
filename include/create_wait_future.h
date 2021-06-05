#pragma once

#include <Future.h>

#ifndef ARDUINO
#include <native/millis.h>
#endif

namespace uniuno {

AsyncResult<void> get_wait_async_state(unsigned long timeout_time) {
  if (timeout_time <= millis()) {
    return AsyncResult<void>::resolve();
  }

  return AsyncResult<void>::pending();
}

template <typename I>
Future<I, void> create_wait_future(unsigned long timeout_ms) {
  unsigned long timeout_time = millis() + timeout_ms;
  return Future<I, void>(
      [timeout_time](I input) { return get_wait_async_state(timeout_time); });
}

template <> Future<void, void> create_wait_future(unsigned long timeout_ms) {
  unsigned long timeout_time = millis() + timeout_ms;
  return Future<void, void>(
      [timeout_time]() { return get_wait_async_state(timeout_time); });
}

} // namespace uniuno