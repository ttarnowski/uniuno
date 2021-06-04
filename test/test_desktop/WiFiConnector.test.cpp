#include <ArduinoFake.h>
#include <WiFiAdapter.hpp>
#include <WiFiConnector.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class WiFiAdapterMock : public WiFiAdapter {
public:
  MOCK_METHOD(bool, mode, (WiFiMode_t m), (override));
  MOCK_METHOD(WiFiMode_t, get_mode, (), (override));
  MOCK_METHOD(bool, add_access_point,
              (const char *ssid, const char *passphrase), (override));
  MOCK_METHOD(wl_status_t, status, (), (override));
  MOCK_METHOD(wl_status_t, run, (uint32_t connectTimeoutMs), (override));
  MOCK_METHOD(bool, disconnect, (bool wifioff), (override));
};

TEST(WiFiConnector, test_connect_resolves_if_WiFi_is_already_connected) {
  WiFiAdapterMock mock;
  WiFiConnector wifi_connector(&mock);

  EXPECT_CALL(mock, get_mode()).Times(1).WillRepeatedly(Return(WIFI_STA));
  EXPECT_CALL(mock, status()).Times(1).WillRepeatedly(Return(WL_CONNECTED));

  auto future = wifi_connector.connect();

  while (future.poll().is_pending())
    ;

  ASSERT_TRUE(future.result.is_resolved());
}

TEST(
    WiFiConnector,
    test_connect_resolves_if_WiFi_is_not_already_connected_but_manages_to_connect) {
  WiFiAdapterMock mock;

  EXPECT_CALL(mock, get_mode()).Times(1).WillRepeatedly(Return(WIFI_STA));
  EXPECT_CALL(mock, status()).Times(1).WillRepeatedly(Return(WL_DISCONNECTED));
  EXPECT_CALL(mock, run(_)).Times(1).WillRepeatedly(Return(WL_CONNECTED));

  WiFiConnector wifi_connector(&mock);

  auto future = wifi_connector.connect(40);

  while (future.poll().is_pending())
    ;

  ASSERT_TRUE(future.result.is_resolved());
}

TEST(WiFiConnector,
     test_connect_sets_wifi_mode_to_WIFI_STA_if_different_mode_has_been_set) {
  WiFiAdapterMock mock;
  WiFiConnector wifi_connector(&mock);

  EXPECT_CALL(mock, get_mode()).WillRepeatedly(Return(WIFI_AP_STA));
  EXPECT_CALL(mock, mode(Eq(WIFI_STA))).Times(1);

  wifi_connector.connect();
}

TEST(
    WiFiConnector,
    test_connect_rejects_with_timeout_error_if_WiFi_connection_cannot_be_estabilished) {
  unsigned long connection_attempt_ms = 20;
  unsigned long connection_timeout_ms = 50;
  WiFiAdapterMock mock;
  WiFiConnector wifi_connector(&mock, connection_attempt_ms);

  EXPECT_CALL(mock, get_mode()).Times(1).WillRepeatedly(Return(WIFI_STA));
  EXPECT_CALL(mock, status()).WillRepeatedly(Return(WL_DISCONNECTED));
  EXPECT_CALL(mock, run(Eq(connection_attempt_ms)))
      .WillRepeatedly(Return(WL_DISCONNECTED));

  auto future = wifi_connector.connect(connection_timeout_ms);

  while (future.poll().is_pending())
    ;

  ASSERT_TRUE(future.result.is_rejected());
  ASSERT_STREQ(*future.result.get_error(), ERROR_FUTURE_TIMEOUT);
}

TEST(
    WiFiConnector,
    test_connect_with_credentials_provided_adds_access_point_and_connects_to_wifi_if_given_credentials_has_been_correct) {
  WiFiAdapterMock mock;
  WiFiConnector wifi_connector(&mock);
  const char *ssid = "ssid";
  const char *password = "password";

  EXPECT_CALL(mock, get_mode()).Times(1).WillRepeatedly(Return(WIFI_STA));
  EXPECT_CALL(mock, status()).Times(1).WillRepeatedly(Return(WL_CONNECTED));
  EXPECT_CALL(mock, add_access_point(Eq(ssid), Eq(password)))
      .Times(1)
      .WillRepeatedly(Return(true));

  auto future = wifi_connector.connect(ssid, password);
  while (future.poll().is_pending())
    ;

  ASSERT_TRUE(future.result.is_resolved());
}

TEST(
    WiFiConnector,
    test_connect_with_credentials_provided_rejects_with_ERROR_INVALID_ACCESS_POINT_if_given_credentials_have_been_incorrect) {
  WiFiAdapterMock mock;
  WiFiConnector wifi_connector(&mock);
  const char *ssid = "super-loooooooooong-sssssssssssid-that-is-not-alloooowed";
  const char *password = "password";

  EXPECT_CALL(mock, get_mode()).WillRepeatedly(Return(WIFI_STA));
  EXPECT_CALL(mock, status()).WillRepeatedly(Return(WL_CONNECTED));
  EXPECT_CALL(mock, add_access_point(Eq(ssid), Eq(password)))
      .Times(1)
      .WillRepeatedly(Return(false));

  auto future = wifi_connector.connect(ssid, password);
  while (future.poll().is_pending())
    ;

  ASSERT_TRUE(future.result.is_rejected());
  ASSERT_STREQ(*future.result.get_error(), ERROR_INVALID_ACCESS_POINT);
}