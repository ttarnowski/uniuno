#pragma once

#include <ESP8266HTTPClient.h>
#include <EventDispatcher.hpp>
#include <WiFiManager.hpp>

class BodyStream : public Stream {
public:
  BodyStream(WiFiClient *wifiClient, HTTPClient *httpClient) {
    this->wifiClient = wifiClient;
    this->httpClient = httpClient;
    this->bytesLeft = this->httpClient->getSize();
  }
  ~BodyStream() {}

  int available() {
    if (!this->httpClient->connected()) {
      return 0;
    }

    return this->bytesLeft;
  }

  size_t readBytes(uint8_t *buffer, size_t length) {
    if (this->bytesLeft == 0) {
      return 0;
    }

    int bytesRead = this->wifiClient->readBytes(
        buffer, std::min((size_t)this->bytesLeft, length));

    if (this->bytesLeft > 0) {
      this->bytesLeft -= bytesRead;
    }

    return bytesRead;
  }

  size_t write(uint8_t buffer) { return this->wifiClient->write(buffer); }
  int read() { return this->wifiClient->read(); }
  int peek() { return this->wifiClient->peek(); }

  String readString() { return this->wifiClient->readString(); }

private:
  WiFiClient *wifiClient;
  HTTPClient *httpClient;
  int bytesLeft;
};

class SingleHostHTTPSClient {
public:
  struct Request {
    const char *path;
  };

  struct Response {
    const char *error;
    int statusCode;
    BodyStream *body;
  };

  SingleHostHTTPSClient(const char *host, const char *pemCert,
                        WiFiManager *wifiManager, Timer *timer) {
    this->host = host;
    this->pemCert = pemCert;
    this->wifiManager = wifiManager;
    this->timer = timer;
  }

  void sendRequest(Request request, std::function<void(Response)> onResponse) {
    this->wifiManager->connect([=](wl_status_t status) {
      if (status != WL_CONNECTED) {
        onResponse(Response{"could not connect to WiFi", -1, nullptr});
        return;
      }

      this->setClock([request, onResponse, this](bool success) {
        if (!success) {
          onResponse(Response{"could not synchronize the time", -1, nullptr});
          return;
        }

        BearSSL::WiFiClientSecure client;
        BearSSL::X509List list(this->pemCert);
        client.setTrustAnchors(&list);

        HTTPClient http;

        Serial.print("[HTTP] begin...\n");

        char url[strlen(this->host) + strlen(request.path) +
                 strlen("https://") + 1];

        strcpy(url, "https://");
        strcat(url, this->host);
        strcat(url, request.path);

        if (http.begin(client, url)) {

          Serial.printf("[HTTP] GET %s\n", url);
          // start connection and send HTTP header, set the HTTP method and
          // request body
          int httpCode = http.GET();

          // httpCode will be negative on error
          if (httpCode > 0) {
            // HTTP header has been send and Server response header has been
            // handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            BodyStream body(&client, &http);

            onResponse(Response{nullptr, httpCode, &body});
          } else {
            // print out the error message
            Serial.printf("[HTTP] GET... failed, error: %s\n",
                          http.errorToString(httpCode).c_str());
            onResponse(Response{http.errorToString(httpCode).c_str(), httpCode,
                                nullptr});
          }

          // finish the exchange
          http.end();
        } else {
          Serial.printf("[HTTP] Unable to connect\n");
          onResponse(Response{"unable to connect", -1, nullptr});
        }
      });
    });
  }

private:
  void setClock(std::function<void(bool)> onClockSet,
                unsigned long timeoutMs = 60000) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    Serial.println("Waiting for NTP time sync: ");

    this->timer->setOnLoopUntil(
        [onClockSet]() {
          time_t now = time(nullptr);
          if (now >= 8 * 3600 * 2) {
            Serial.println("");
            struct tm timeinfo;
            gmtime_r(&now, &timeinfo);
            Serial.print("Current time: ");
            Serial.print(asctime(&timeinfo));
            onClockSet(true);
            return true;
          }

          return false;
        },
        [onClockSet]() { onClockSet(false); }, timeoutMs);
  }

  const char *host;
  const char *pemCert;
  WiFiManager *wifiManager;
  Timer *timer;
};
