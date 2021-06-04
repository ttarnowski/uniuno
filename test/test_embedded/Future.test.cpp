#include <Arduino.h>
#include <Future.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(Future, test_futures_work_on_embedded_environment) {
  unsigned long time = millis() + 50;

  Future<int, int> f1([time](int val) {
    if (millis() >= time) {
      return AsyncResult<int>::resolve(val * 3);
    }

    return AsyncResult<int>::pending();
  });

  Future<int, bool> f2(
      [](int val) { return AsyncResult<bool>::resolve(val % 2 == 0); });

  auto f =
      f1.and_then(f2).and_then([](bool val) { return val ? "true" : "false"; });

  auto g = f;

  while (f.poll(2).is_pending() || g.poll(3).is_pending())
    ;

  ASSERT_EQ(*f.result.get_value(), "true");
  ASSERT_EQ(*g.result.get_value(), "false");
  ASSERT_GE(millis(), time);
}