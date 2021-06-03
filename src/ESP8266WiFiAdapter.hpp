#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiAdapter.hpp>

class ESP8266WiFiAdapter : public WiFiAdapter {
public:
  ESP8266WiFiAdapter(ESP8266WiFiClass *wifi, ESP8266WiFiMulti *wifiMulti) {
    this->wifi = wifi;
    this->wifiMulti = wifiMulti;
  }

  bool mode(WiFiMode_t m) override { return this->wifi->mode(m); }

  WiFiMode_t getMode() override { return this->wifi->getMode(); }

  bool addAP(const char *ssid, const char *passphrase) override {
    return this->wifiMulti->addAP(ssid, passphrase);
  }

  wl_status_t status() override { return this->wifi->status(); }

  wl_status_t run(uint32_t connectTimeoutMs = 5000) override {
    return this->wifiMulti->run(connectTimeoutMs);
  }

  bool disconnect(bool wifioff = false) override {
    return this->wifi->disconnect(wifioff);
  }

private:
  ESP8266WiFiClass *wifi;
  ESP8266WiFiMulti *wifiMulti;
};