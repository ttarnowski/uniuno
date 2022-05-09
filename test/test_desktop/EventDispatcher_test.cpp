#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <EventDispatcher.hpp>

using namespace testing;

struct TestEvent {
  static constexpr const char *Name = "Test";
  const char *id;
};

class Callback {
  virtual void call(TestEvent *e) = 0;
};

class CallbackMock : public Callback {
public:
  MOCK_METHOD(void, call, (TestEvent * e), (override));
};

TEST(EventDispatcher,
     on_test_event_is_called_whenever_test_event_is_dispatched) {
  EventDispatcher dispatcher;
  CallbackMock mock;
  TestEvent expectedEvent1{"abcd"};
  TestEvent expectedEvent2{"bcde"};

  {
    InSequence s;
    EXPECT_CALL(mock, call(Field(&TestEvent::id, Eq(expectedEvent1.id))));
    EXPECT_CALL(mock, call(Field(&TestEvent::id, Eq(expectedEvent2.id))));
  }

  dispatcher.on<TestEvent>([&](TestEvent *e) { mock.call(e); });

  dispatcher.dispatch<TestEvent>(expectedEvent1);
  dispatcher.dispatch<TestEvent>(expectedEvent2);
}

TEST(EventDispatcher,
     once_test_event_is_called_once_despite_test_event_being_dispatched_twice) {
  EventDispatcher dispatcher;
  CallbackMock mock;
  TestEvent expectedEvent1{"abcdef"};

  EXPECT_CALL(mock, call(Field(&TestEvent::id, Eq(expectedEvent1.id))))
      .Times(1);

  dispatcher.once<TestEvent>([&](TestEvent *e) { mock.call(e); });

  dispatcher.dispatch<TestEvent>(expectedEvent1);
  dispatcher.dispatch<TestEvent>(TestEvent{"abcdef"});
}
