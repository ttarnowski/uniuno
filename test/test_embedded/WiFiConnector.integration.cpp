#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAdapter.hpp>
#include <ESP8266WiFiMulti.h>
#include <WiFiConnector.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef TEST_INTEGRATION

#define XSTR(x) #x
#define STR(x) XSTR(x)

const char *ssid = STR(SSID);
const char *password = STR(PASSWORD);

class Callback {
  virtual void call() = 0;
};

class CallbackMock : public Callback {
public:
  MOCK_METHOD(void, call, (), (override));
};

TEST(WiFiConnector, test_connect_connects_to_wifi) {
  CallbackMock mock;
  ESP8266WiFiMulti wifi_multi;
  ESP8266WiFiAdapter wifi_adapter(&WiFi, &wifi_multi);

  WiFiConnector wifi_connector(&wifi_adapter);

  EXPECT_CALL(mock, call());

  auto future =
      wifi_connector.connect(ssid, password, 60 * 1000).and_then([&mock]() {
        mock.call();
      });

  while (future.poll().is_pending()) {
    yield();
  }

  ASSERT_TRUE(future.result.is_resolved());
}

#endif