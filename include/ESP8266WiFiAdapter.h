#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiAdapter.h>

namespace uniuno {

class ESP8266WiFiAdapter : public WiFiAdapter {
public:
  ESP8266WiFiAdapter() : wifi_multi() {}

  bool mode(WiFiMode_t m) override { return WiFi.mode(m); }

  WiFiMode_t get_mode() override { return WiFi.getMode(); }

  bool add_access_point(const char *ssid, const char *passphrase) override {
    return this->wifi_multi.addAP(ssid, passphrase);
  }

  wl_status_t status() override { return WiFi.status(); }

  wl_status_t run(uint32_t connectTimeoutMs = 5000) override {
    return this->wifi_multi.run(connectTimeoutMs);
  }

  bool disconnect(bool wifioff = false) override {
    return WiFi.disconnect(wifioff);
  }

private:
  ESP8266WiFiMulti wifi_multi;
};

} // namespace uniuno