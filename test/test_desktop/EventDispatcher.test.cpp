#include <EventDispatcher.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace uniuno;

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

TEST(
    EventDispatcher,
    test_listeners_set_with_on_method_have_been_notified_for_each_dispatched_event_of_the_same_type) {
  EventDispatcher dispatcher;
  TestEvent e1{"123"};
  TestEvent e2{"234"};
  CallbackMock mock;

  {
    InSequence s;
    EXPECT_CALL(mock, call(Field(&TestEvent::id, Eq(e1.id)))).Times(2);
    EXPECT_CALL(mock, call(Field(&TestEvent::id, Eq(e2.id)))).Times(2);
  }

  dispatcher.on<TestEvent>([&](TestEvent *e) { mock.call(e); });
  dispatcher.on<TestEvent>([&](TestEvent *e) { mock.call(e); });

  dispatcher.dispatch<TestEvent>(e1);
  dispatcher.dispatch<TestEvent>(e2);
}

TEST(
    EventDispatcher,
    test_listeners_set_with_once_method_have_been_notified_only_once_when_there_was_an_event_of_the_same_type_dispatched) {
  EventDispatcher dispatcher;
  TestEvent e1{"123"};
  TestEvent e2{"234"};
  CallbackMock mock;

  EXPECT_CALL(mock, call(Field(&TestEvent::id, Eq(e1.id)))).Times(2);

  dispatcher.once<TestEvent>([&](TestEvent *e) { mock.call(e); });
  dispatcher.once<TestEvent>([&](TestEvent *e) { mock.call(e); });

  dispatcher.dispatch<TestEvent>(e1);
  dispatcher.dispatch<TestEvent>(e2);
}
