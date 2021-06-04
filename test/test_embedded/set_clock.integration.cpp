#ifdef TEST_INTEGRATION

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAdapter.hpp>
#include <ESP8266WiFiMulti.h>
#include <WiFiConnector.hpp>
#include <create_timeout_future.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <set_clock.hpp>

#define XSTR(x) #x
#define STR(x) XSTR(x)

TEST(set_clock, test_it_times_when_the_time_cannot_be_synchronized) {
  ESP8266WiFiMulti wifi_multi;
  ESP8266WiFiAdapter wifi_adapter(&WiFi, &wifi_multi);

  WiFiConnector wifi_connector(&wifi_adapter);

  const char *ssid = STR(SSID);
  const char *password = STR(PASSWORD);
  wifi_connector.disconnect();
  wifi_connector.add_access_point(ssid, password);

  auto future = set_clock(&wifi_connector, 0);

  while (future.poll().is_pending()) {
    yield();
  }

  ASSERT_TRUE(future.result.is_rejected());
  ASSERT_STREQ(*future.result.get_error(), ERROR_FUTURE_TIMEOUT);
}

TEST(set_clock, test_it_synchronizes_the_clock_if_it_can_connect_to_internet) {
  ESP8266WiFiMulti wifi_multi;
  ESP8266WiFiAdapter wifi_adapter(&WiFi, &wifi_multi);

  WiFiConnector wifi_connector(&wifi_adapter);

  const char *ssid = STR(SSID);
  const char *password = STR(PASSWORD);
  wifi_connector.disconnect();
  wifi_connector.add_access_point(ssid, password);

  auto future = set_clock(&wifi_connector);

  while (future.poll().is_pending()) {
    yield();
  }

  ASSERT_TRUE(future.result.is_resolved());
}

#endif