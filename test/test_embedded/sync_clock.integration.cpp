#ifdef TEST_INTEGRATION

#include <Arduino.h>
#include <ESP8266WiFiAdapter.h>
#include <WiFiConnector.h>
#include <create_timeout_future.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sync_clock.h>

#define XSTR(x) #x
#define STR(x) XSTR(x)

using namespace uniuno;

TEST(sync_clock, test_it_times_when_the_time_cannot_be_synchronized) {
  ESP8266WiFiAdapter wifi_adapter;
  WiFiConnector wifi_connector(&wifi_adapter);

  const char *ssid = STR(SSID);
  const char *password = STR(PASSWORD);
  wifi_connector.disconnect();
  wifi_connector.add_access_point(ssid, password);

  auto future = sync_clock(&wifi_connector, 0);

  while (future.poll().is_pending()) {
    yield();
  }

  ASSERT_TRUE(future.result.is_rejected());
  ASSERT_STREQ(*future.result.get_error(), ERROR_FUTURE_TIMEOUT);
}

TEST(sync_clock, test_it_synchronizes_the_clock_if_it_can_connect_to_internet) {
  ESP8266WiFiAdapter wifi_adapter;
  WiFiConnector wifi_connector(&wifi_adapter);

  const char *ssid = STR(SSID);
  const char *password = STR(PASSWORD);
  wifi_connector.disconnect();
  wifi_connector.add_access_point(ssid, password);

  auto future = sync_clock(&wifi_connector);

  while (future.poll().is_pending()) {
    yield();
  }

  ASSERT_TRUE(future.result.is_resolved());
}

#endif