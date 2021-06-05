#include <Arduino.h>
#include <Executor.h>
#include <Future.h>
#include <create_wait_future.h>
#include <unity.h>

using namespace uniuno;

Executor executor;
unsigned long t1;
unsigned long t2;

unsigned long timeout = 500;

bool has_resolve_been_called = false;
bool has_reject_been_called = false;

const char *expected_error = "expected";
char actual_error[10];

void test_executor_fullfills_successful_future(void) {
  TEST_ASSERT_TRUE(has_resolve_been_called);
}

void test_executor_fullfills_failed_future(void) {
  TEST_ASSERT_EQUAL_STRING(expected_error, actual_error);
}

void test_executor_fullfils_futures_in_order(void) {
  TEST_ASSERT_FALSE(has_reject_been_called && !has_resolve_been_called);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  UNITY_BEGIN();

  t1 = millis() + timeout;
  t2 = t1 + 1;

  executor.execute(create_wait_future(timeout).and_then(
      []() { has_resolve_been_called = true; }));

  executor.execute(Future<void, void>::reject(Error(expected_error)),
                   [](Error e) {
                     Serial.println("rejected");
                     strcpy(actual_error, e);
                     has_reject_been_called = true;
                   });
}

void loop() {
  RUN_TEST(test_executor_fullfils_futures_in_order);

  if (millis() > t1) {
    RUN_TEST(test_executor_fullfills_successful_future);
  }

  if (millis() > t2) {
    RUN_TEST(test_executor_fullfills_failed_future);
    UNITY_END();
  }
}