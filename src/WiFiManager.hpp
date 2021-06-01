#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <EventDispatcher.hpp>
#include <Future.hpp>
#include <WiFiClient.h>

class WiFiManager {

public:
  struct WiFiConnectedEvent {
    static constexpr const char *Name = "wifi_connected";
  };

  struct WiFiDisconnectedEvent {
    static constexpr const char *Name = "wifi_disconnected";
  };

  WiFiManager(ESP8266WiFiClass *wifi, ESP8266WiFiMulti *wifi_multi,
              EventDispatcher *dispatcher, const char *ssid,
              const char *password) {
    this->wifi = wifi;
    this->wifi_multi = wifi_multi;
    this->dispatcher = dispatcher;

    this->wifi->mode(WIFI_STA);
    this->wifi_multi->addAP(ssid, password);
  }

  Future<void, wl_status_t> connect(unsigned long timeout_ms = 5000) {
    return Future<void, wl_status_t>([this]() {
      if (this->wifi->status() == WL_CONNECTED) {
        return AsyncResult<wl_status_t>::resolve(WL_CONNECTED);
      }

      if (this->wifi_multi->run() == WL_CONNECTED) {
        this->dispatcher->dispatch(WiFiConnectedEvent{});
        return AsyncResult<wl_status_t>::resolve(WL_CONNECTED);
      }

      return AsyncResult<wl_status_t>::pending();
    });
  }

  Future<void, void> disconnect() {
    return Future<void, void>([this]() {
      if (this->wifi->status() == WL_DISCONNECTED) {
        this->dispatcher->dispatch(WiFiDisconnectedEvent{});
        return AsyncResult<void>::resolve();
      }

      return AsyncResult<void>::pending();
    });
  }

private:
  ESP8266WiFiMulti *wifi_multi;
  EventDispatcher *dispatcher;
  ESP8266WiFiClass *wifi;
  bool connected = false;
};