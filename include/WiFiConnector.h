#pragma once

#include <Future.h>
#include <WiFiAdapter.h>
#include <create_timeout_future.h>
#include <logging.h>

#define ERROR_INVALID_ACCESS_POINT "could not add access point"

namespace uniuno {

class WiFiConnector {

public:
  WiFiConnector(WiFiAdapter *wifi, unsigned long connection_attempt_ms = 2000) {
    this->wifi = wifi;
    this->connection_attempt_ms = connection_attempt_ms;
  }

  bool add_access_point(const char *ssid, const char *password) {
    return this->wifi->add_access_point(ssid, password);
  }

  Future<void, void> connect(unsigned long timeout_ms = 15000) {
    if (this->wifi->get_mode() != WIFI_STA) {
      this->wifi->mode(WIFI_STA);
    }

    return create_timeout_future(
        create_future([this]() {
          TRACE("checking wifi status");
          if (this->wifi->status() == WL_CONNECTED) {
            DEBUG("wifi status: connected");
            return AsyncResult<void>::resolve();
          }
          TRACE("not connected, scanning");

          if (this->wifi->run(this->connection_attempt_ms) == WL_CONNECTED) {
            DEBUG("wifi connected");
            return AsyncResult<void>::resolve();
          }

          TRACE("connecting to wifi in progress");
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

} // namespace uniuno