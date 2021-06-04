#include <Future.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

TEST(Future, test_result_has_default_state_pending_for_all_types_of_futures) {
  Future<int, int> f1([](int) { return AsyncResult<int>::pending(); });
  Future<void, int> f2([]() { return AsyncResult<int>::pending(); });
  Future<int, void> f3([](int) { return AsyncResult<void>::pending(); });
  Future<void, void> f4([]() { return AsyncResult<void>::pending(); });

  ASSERT_TRUE(f1.result.is_pending());
  ASSERT_TRUE(f2.result.is_pending());
  ASSERT_TRUE(f3.result.is_pending());
  ASSERT_TRUE(f4.result.is_pending());
}

TEST(
    Future,
    test_future_changes_its_result_state_to_resolved_if_poll_fn_returns_resolved_result) {
  int expected = 1;

  Future<int, int> f1([](int val) { return AsyncResult<int>::resolve(val); });
  Future<void, int> f2(
      [expected]() { return AsyncResult<int>::resolve(expected); });
  Future<int, void> f3([](int) { return AsyncResult<void>::resolve(); });
  Future<void, void> f4([]() { return AsyncResult<void>::resolve(); });

  ASSERT_TRUE(f1.poll(expected).is_resolved());
  ASSERT_TRUE(f2.poll().is_resolved());
  ASSERT_TRUE(f3.poll(1).is_resolved());
  ASSERT_TRUE(f4.poll().is_resolved());

  ASSERT_TRUE(f1.result.is_resolved());
  ASSERT_TRUE(f2.result.is_resolved());
  ASSERT_TRUE(f3.result.is_resolved());
  ASSERT_TRUE(f4.result.is_resolved());

  ASSERT_EQ(*f1.result.get_value(), expected);
  ASSERT_EQ(*f2.result.get_value(), expected);
}

TEST(Future,
     test_futures_chain_is_pending_when_first_future_in_chain_is_pending) {

  Future<int, int> f1([](int) { return AsyncResult<int>::pending(); });
  Future<void, int> f2([]() { return AsyncResult<int>::pending(); });
  Future<int, void> f3([](int) { return AsyncResult<void>::pending(); });
  Future<void, void> f4([]() { return AsyncResult<void>::pending(); });

  auto c1 = f1.and_then(
      Future<int, void>([](int) { return AsyncResult<void>::resolve(); }));
  auto c2 = f2.and_then(
      Future<int, void>([](int) { return AsyncResult<void>::resolve(); }));
  auto c3 = f3.and_then(
      Future<void, void>([]() { return AsyncResult<void>::resolve(); }));
  auto c4 = f4.and_then(
      Future<void, void>([]() { return AsyncResult<void>::resolve(); }));

  ASSERT_TRUE(c1.poll(2).is_pending());
  ASSERT_TRUE(c1.result.is_pending());
  ASSERT_TRUE(c2.poll().is_pending());
  ASSERT_TRUE(c2.result.is_pending());
  ASSERT_TRUE(c3.poll(2).is_pending());
  ASSERT_TRUE(c3.result.is_pending());
  ASSERT_TRUE(c4.poll().is_pending());
  ASSERT_TRUE(c4.result.is_pending());
}

TEST(
    Future,
    test_futures_chain_resolves_with_last_future_return_value_if_all_previous_futures_in_chain_have_resolved) {
  int input = 3;
  int expected = input * 2;

  Future<int, int> f1([](int val) { return AsyncResult<int>::resolve(val); });
  Future<void, int> f2(
      [expected]() { return AsyncResult<int>::resolve(expected); });
  Future<int, void> f3([](int val) { return AsyncResult<void>::resolve(); });
  Future<void, void> f4([]() { return AsyncResult<void>::resolve(); });

  auto c1 = f1.and_then(Future<int, int>(
      [](int val) { return AsyncResult<int>::resolve(val * 2); }));
  auto c2 = f2.and_then(Future<int, int>(
      [expected](int) { return AsyncResult<int>::resolve(expected); }));
  auto c3 = f3.and_then(Future<void, int>(
      [expected]() { return AsyncResult<int>::resolve(expected); }));
  auto c4 = f4.and_then(Future<void, int>(
      [expected]() { return AsyncResult<int>::resolve(expected); }));

  ASSERT_TRUE(c1.poll(input).is_resolved());
  ASSERT_EQ(*c1.result.get_value(), expected);
  ASSERT_TRUE(c2.poll().is_resolved());
  ASSERT_EQ(*c2.result.get_value(), expected);
  ASSERT_TRUE(c3.poll(input).is_resolved());
  ASSERT_EQ(*c3.result.get_value(), expected);
  ASSERT_TRUE(c4.poll().is_resolved());
  ASSERT_EQ(*c4.result.get_value(), expected);
}

TEST(
    Future,
    test_long_futures_chain_resolves_with_expected_value_after_expected_amount_of_waiting_time) {
  auto millis = []() {
    return std::chrono::system_clock::now().time_since_epoch() /
           std::chrono::milliseconds(1);
  };

  unsigned long time = millis() + 50;

  Future<int, int> f1([time, millis](int val) {
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

class Callback {
  virtual void call(int actual) = 0;
};

class CallbackMock : public Callback {
public:
  MOCK_METHOD(void, call, (int actual), (override));
};

TEST(Future, test_void_and_non_void_chaining_for_futures) {
  CallbackMock mock;
  int expected = 3;
  Future<void, void> f1([]() { return AsyncResult<void>::resolve(); });

  EXPECT_CALL(mock, call(Eq(expected)));

  auto f = f1.and_then(Future<void, int>([expected]() {
               return AsyncResult<int>::resolve(expected);
             }))
               .and_then(Future<int, void>([&mock](int actual) {
                 mock.call(actual);
                 return AsyncResult<void>::resolve();
               }));

  while (f.poll().is_pending())
    ;

  ASSERT_TRUE(f.result.is_resolved());
}

TEST(Future, test_void_and_non_void_chaining_for_callbacks) {
  CallbackMock mock;
  int expected = 4;

  EXPECT_CALL(mock, call(Eq(expected))).Times(3);

  Future<void, int> f1(
      [expected]() { return AsyncResult<int>::resolve(expected); });

  auto f = f1.and_then([&mock](int actual) { mock.call(actual); })
               .and_then([expected]() { return expected; })
               .and_then([&mock](int actual) { mock.call(actual); })
               .and_then([&mock, expected]() { mock.call(expected); });

  while (f.poll().is_pending())
    ;

  ASSERT_TRUE(f.result.is_resolved());
}

TEST(Future, test_error_handling_with_default_error) {
  CallbackMock mock;
  const char *expected = "expected error";

  EXPECT_CALL(mock, call(_)).Times(0);

  Future<void, int> f1(
      [expected]() { return AsyncResult<int>::reject(Error(expected)); });

  auto f = f1.and_then(Future<int, int>([&mock](int val) {
               mock.call(val);

               return val;
             }))
               .and_then([&mock](int val) { mock.call(val); });

  while (f.poll().is_pending())
    ;

  ASSERT_TRUE(f.result.is_rejected());
  ASSERT_STREQ(*f.result.get_error(), expected);
}

TEST(Future, test_error_handling_with_custom_error_in_first_future) {
  class CustomError : ErrorBase {
  public:
    CustomError() { strcpy(this->err, "error : 0"); }

    CustomError(const char *err, int code = 0) {
      sprintf(this->err, "%s : %d", err, code);
    }

    operator const char *() override { return this->err; }

  private:
    char err[64];
  };

  CallbackMock mock;
  const char *error_message = "expected error";
  int error_code = 123;
  char expected[32];
  sprintf(expected, "%s : %d", error_message, error_code);

  EXPECT_CALL(mock, call(_)).Times(0);

  Future<void, int, CustomError> f1([expected, error_message, error_code]() {
    return AsyncResult<int, CustomError>::reject(
        CustomError(error_message, error_code));
  });

  auto f = f1.and_then(Future<int, int>([&mock](int val) {
               mock.call(val);

               return val;
             }))
               .and_then([&mock](int val) { mock.call(val); });

  while (f.poll().is_pending())
    ;

  ASSERT_TRUE(f.result.is_rejected());
  ASSERT_STREQ(*f.result.get_error(), expected);
}

TEST(Future, test_error_handling_with_custom_error_in_last_future) {
  class CustomError : ErrorBase {
  public:
    CustomError() { strcpy(this->err, "error : 0"); }

    CustomError(const char *err, int code = 0) {
      sprintf(this->err, "%s : %d", err, code);
    }

    operator const char *() override { return this->err; }

  private:
    char err[64];
  };

  const char *error_message = "expected error";
  int error_code = 123;
  char expected[32];
  sprintf(expected, "%s : %d", error_message, error_code);

  Future<void, void> f1([]() { return AsyncResult<void>::resolve(); });

  Future<void, int, CustomError> f2([expected, error_message, error_code]() {
    return AsyncResult<int, CustomError>::reject(
        CustomError(error_message, error_code));
  });

  auto f = f1.and_then(f2);

  while (f.poll().is_pending())
    ;

  ASSERT_TRUE(f.result.is_rejected());
  ASSERT_STREQ(*f.result.get_error(), expected);
}
