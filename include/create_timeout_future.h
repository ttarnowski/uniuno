#pragma once

#include <Future.h>

#ifndef ARDUINO
#include <native/millis.h>
#endif

#define ERROR_FUTURE_TIMEOUT "operation timed out"

namespace uniuno {

template <typename I, typename O, typename E = Error>
Future<I, O, E> create_timeout_future(Future<I, O, E> future,
                                      unsigned long timeout_ms = 5000) {
  unsigned long timeout_time = millis() + timeout_ms;
  return Future<I, O, E>([future, timeout_time](I input) mutable {
    if (millis() >= timeout_time && future.result.is_pending()) {
      return AsyncResult<O, E>::reject(E(ERROR_FUTURE_TIMEOUT));
    }

    return future.poll(input);
  });
}

template <typename O, typename E = Error>
Future<void, O, E> create_timeout_future(Future<void, O, E> future,
                                         unsigned long timeout_ms = 5000) {
  unsigned long timeout_time = millis() + timeout_ms;
  return Future<void, O, E>([future, timeout_time]() mutable {
    if (millis() >= timeout_time && future.result.is_pending()) {
      return AsyncResult<O, E>::reject(E(ERROR_FUTURE_TIMEOUT));
    }

    return future.poll();
  });
}

} // namespace uniuno