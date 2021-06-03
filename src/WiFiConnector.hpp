#pragma once

#include <EventDispatcher.hpp>
#include <Future.hpp>
#include <WiFiAdapter.hpp>
#include <create_timeout_future.hpp>
#include <logging.h>

#define ERROR_INVALID_ACCESS_POINT "could not add access point"

class WiFiConnector {

public:
  struct WiFiConnectedEvent {
    static constexpr const char *Name = "wifi_connected";
  };

  struct WiFiDisconnectedEvent {
    static constexpr const char *Name = "wifi_disconnected";
  };

  WiFiConnector(WiFiAdapter *wifi, unsigned long connection_attempt_ms = 5000) {
    this->wifi = wifi;
    this->connection_attempt_ms = connection_attempt_ms;
  }

  bool add_access_point(const char *ssid, const char *password) {
    return this->wifi->addAP(ssid, password);
  }

  Future<void, void> connect(unsigned long timeout_ms = 15000) {
    if (this->wifi->getMode() != WIFI_STA) {
      this->wifi->mode(WIFI_STA);
    }

    return create_timeout_future(
        create_future([this]() {
          if (this->wifi->status() == WL_CONNECTED) {
            return AsyncResult<void>::resolve();
          }

          if (this->wifi->run(this->connection_attempt_ms) == WL_CONNECTED) {
            return AsyncResult<void>::resolve();
          }

          return AsyncResult<void>::pending();
        }),
        timeout_ms);
  }

  Future<void, void> connect(const char *ssid, const char *password,
                             unsigned long timeout_ms = 15000) {
    if (!this->add_access_point(ssid, password)) {
      return Future<void, void>::reject(Error(ERROR_INVALID_ACCESS_POINT));
    }

    return this->connect(timeout_ms);
  }

  bool disconnect() { return this->wifi->disconnect(); }

private:
  WiFiAdapter *wifi;
  unsigned long connection_attempt_ms;
};