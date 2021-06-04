#pragma once

#ifndef ARDUINO

typedef enum WiFiMode {
  WIFI_OFF = 0,
  WIFI_STA = 1,
  WIFI_AP = 2,
  WIFI_AP_STA = 3,
  /* these two pseudo modes are experimental: */ WIFI_SHUTDOWN = 4,
  WIFI_RESUME = 8
} WiFiMode_t;

typedef enum {
  WL_NO_SHIELD = 255, // for compatibility with WiFi Shield library
  WL_IDLE_STATUS = 0,
  WL_NO_SSID_AVAIL = 1,
  WL_SCAN_COMPLETED = 2,
  WL_CONNECTED = 3,
  WL_CONNECT_FAILED = 4,
  WL_CONNECTION_LOST = 5,
  WL_DISCONNECTED = 6
} wl_status_t;

#else

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#endif

class WiFiAdapter {
public:
  virtual bool mode(WiFiMode_t m) = 0;

  virtual WiFiMode_t get_mode() = 0;

  virtual bool add_access_point(const char *ssid, const char *passphrase) = 0;

  virtual wl_status_t status() = 0;

  virtual wl_status_t run(unsigned int connectTimeoutMs = 5000) = 0;

  virtual bool disconnect(bool wifioff = false) = 0;

  virtual ~WiFiAdapter() {}
};