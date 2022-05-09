#include <Timer.hpp>
#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

unsigned long millis() {
  return std::chrono::system_clock::now().time_since_epoch() /
         std::chrono::milliseconds(1);
}

Timer timer(millis);

class TimeoutCallback {
  virtual void call(unsigned long actualTime) = 0;
};

class TimeoutCallbackMock : public TimeoutCallback {
public:
  MOCK_METHOD(void, call, (unsigned long actualTime), (override));
};

unsigned long errorMargin = 3;

TEST(Timer, setTimeout_calls_given_callback_after_given_amount_of_time) {
  TimeoutCallbackMock mock;
  unsigned long interval = 500;
  unsigned long expectedTime = millis() + interval;

  EXPECT_CALL(mock, call(AllOf(Ge(expectedTime - errorMargin),
                               Le(expectedTime + errorMargin))))
      .Times(1);

  timer.setTimeout([&]() { mock.call(millis()); }, interval);

  while (millis() < expectedTime + errorMargin) {
    timer.tick();
  }
}

TEST(Timer, setInterval_calls_given_callback_every_given_amount_of_ms) {
  TimeoutCallbackMock mock;
  unsigned long interval = 100;
  unsigned long actualTime = millis();

  {
    InSequence s;
    EXPECT_CALL(mock, call(AllOf(Ge(actualTime + interval * 1 - errorMargin),
                                 Le(actualTime + interval * 1 + errorMargin))));
    EXPECT_CALL(mock, call(AllOf(Ge(actualTime + interval * 2 - errorMargin),
                                 Le(actualTime + interval * 2 + errorMargin))));
    EXPECT_CALL(mock, call(AllOf(Ge(actualTime + interval * 3 - errorMargin),
                                 Le(actualTime + interval * 3 + errorMargin))));
  }

  timer.setInterval([&]() { mock.call(millis()); }, interval);

  while (millis() < actualTime + 3 * interval + errorMargin * 2) {
    timer.tick();
  }
}
