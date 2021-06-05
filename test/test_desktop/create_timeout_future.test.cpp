#include <Future.h>
#include <create_timeout_future.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace uniuno;

TEST(
    create_timeout_future,
    it_resolves_with_underlying_future_value_if_that_future_resolved_before_timeout) {
  int expected = 123;

  auto f1 = create_timeout_future(Future<void, int>::resolve(expected));
  auto f2 = create_timeout_future(Future<int, int>::resolve(expected));

  while (f1.poll().is_pending() || f2.poll(2).is_pending())
    ;

  ASSERT_TRUE(f1.result.is_resolved());
  ASSERT_TRUE(f2.result.is_resolved());
  ASSERT_EQ(*f1.result.get_value(), expected);
  ASSERT_EQ(*f2.result.get_value(), expected);
}

TEST(
    create_timeout_future,
    it_rejects_with_timeout_error_if_underlying_future_is_still_pending_after_timeout) {
  auto f1 = create_timeout_future(
      create_future([]() { return AsyncResult<void>::pending(); }), 50);
  auto f2 = create_timeout_future(
      create_future([](int input) { return AsyncResult<void>::pending(); }),
      50);

  while (f1.poll().is_pending() || f2.poll(2).is_pending())
    ;

  ASSERT_TRUE(f1.result.is_rejected());
  ASSERT_TRUE(f2.result.is_rejected());
  ASSERT_STREQ(*f1.result.get_error(), ERROR_FUTURE_TIMEOUT);
  ASSERT_STREQ(*f2.result.get_error(), ERROR_FUTURE_TIMEOUT);
}

TEST(
    create_timeout_future,
    it_rejects_with_underlying_future_error_if_that_future_rejected_before_timeout) {
  const char *expected = "error";

  auto f1 = create_timeout_future(Future<void, int>::reject(Error(expected)));
  auto f2 = create_timeout_future(Future<int, int>::reject(Error(expected)));

  while (f1.poll().is_pending() || f2.poll(2).is_pending())
    ;

  ASSERT_TRUE(f1.result.is_rejected());
  ASSERT_TRUE(f2.result.is_rejected());
  ASSERT_STREQ(*f1.result.get_error(), expected);
  ASSERT_STREQ(*f2.result.get_error(), expected);
}
