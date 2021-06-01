#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <EventDispatcher.hpp>
#include <Timer.hpp>
#include <WiFiClient.h>

class WiFiManager {
private:
  ESP8266WiFiMulti *wifiMulti;
  EventDispatcher *dispatcher;
  Timer *timer;
  bool connected = false;

public:
  struct WiFiConnectedEvent {
    static constexpr const char *Name = "wifi_connected";
  };

  struct WiFiDisconnectedEvent {
    static constexpr const char *Name = "wifi_disconnected";
  };

  WiFiManager(ESP8266WiFiMulti *wifiMulti, EventDispatcher *dispatcher,
              Timer *timer, const char *ssid, const char *password) {
    this->wifiMulti = wifiMulti;
    this->dispatcher = dispatcher;
    this->timer = timer;
    WiFi.mode(WIFI_STA);
    this->wifiMulti->addAP(ssid, password);
  }

  void connect(std::function<void(wl_status_t)> onConnected,
               unsigned long timeoutMs = 5000) {
    if (this->connected && WiFi.status() == WL_CONNECTED) {
      onConnected(WL_CONNECTED);
      return;
    }

    this->connected = false;

    this->timer->setOnLoopUntil(
        [onConnected, this]() {
          if (this->wifiMulti->run() == WL_CONNECTED) {
            this->connected = true;
            onConnected(WL_CONNECTED);
            this->dispatcher->dispatch(WiFiConnectedEvent{});
            return true;
          }
          return false;
        },
        [onConnected]() { onConnected(WL_CONNECT_FAILED); }, timeoutMs);
  }

  void disconnect() {
    WiFi.disconnect();
    this->connected = false;
    this->dispatcher->dispatch(WiFiDisconnectedEvent{});
  }
};