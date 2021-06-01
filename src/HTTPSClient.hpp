#pragma once

#include <CertStoreBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <EventDispatcher.hpp>
#include <WiFiManager.hpp>
#include <setClock.hpp>

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

class RequestBuilder;

class Request {
public:
  enum Method { GET, HEAD, POST, PUT, PATCH, DELETE, OPTIONS };

  static RequestBuilder build(Request::Method method, const char *url);
  const char *url;
  Request::Method method;
  Stream *body = nullptr;
  size_t bodySize = 0;
  const char *bodyStr = nullptr;
  std::vector<std::pair<const char *, const char *>> headers;
};

class RequestBuilder {
public:
  RequestBuilder(Request::Method method, const char *url) {
    request.method = method;
    request.url = url;
  }

  operator Request &&() { return std::move(request); }

  RequestBuilder &body(const char *b) {
    request.bodyStr = b;
    request.bodySize = strlen(b);
    request.body = nullptr;
    return *this;
  }

  RequestBuilder &body(Stream *s, size_t size) {
    request.body = s;
    request.bodySize = size;
    request.bodyStr = nullptr;

    return *this;
  }

  RequestBuilder &body(File *file) { return this->body(file, file->size()); }

  RequestBuilder &
  headers(std::vector<std::pair<const char *, const char *>> h) {
    request.headers = h;
    return *this;
  }

private:
  Request request;
};

RequestBuilder Request::build(Request::Method method, const char *url) {
  return RequestBuilder(method, url);
}

struct Response {
  const char *error;
  int statusCode;
  BodyStream *body;
};

class HTTPSClient {
public:
  HTTPSClient(CertStore *certStore, WiFiManager *wifiManager, Timer *timer) {
    this->wifiManager = wifiManager;
    this->timer = timer;
    this->certStore = certStore;
  }

  void sendRequest(Request request, std::function<void(Response)> onResponse) {
    this->wifiManager->connect([=](wl_status_t status) {
      if (status != WL_CONNECTED) {
        onResponse(Response{"could not connect to WiFi", -1, nullptr});
        return;
      }

      setClock(this->timer, [request, onResponse, this](bool success) {
        if (!success) {
          onResponse(Response{"could not synchronize the time", -1, nullptr});
          return;
        }

        BearSSL::WiFiClientSecure client;
        client.setCertStore(this->certStore);

        HTTPClient http;

        Serial.print("[HTTP] begin...\n");

        if (http.begin(client, request.url)) {
          char method[10];
          this->readMethod(request.method, method);

          Serial.printf("[HTTP] %s %s\n", method, request.url);
          // start connection and send HTTP header, set the HTTP method and
          // request body
          for (auto &h : request.headers) {
            http.addHeader(h.first, h.second);
          }

          int httpCode;

          if (request.body == nullptr && request.bodyStr == nullptr) {
            httpCode = http.sendRequest(method, (uint8_t *)nullptr, 0);
          } else if (request.body != nullptr) {
            httpCode = http.sendRequest(method, request.body, request.bodySize);
          } else {
            httpCode = http.sendRequest(method, String(request.bodyStr));
          }

          // httpCode will be negative on error
          if (httpCode > 0) {
            // HTTP header has been send and Server response header has been
            // handled
            Serial.printf("[HTTP] %s... code: %d\n", method, httpCode);

            BodyStream body(&client, &http);

            onResponse(Response{nullptr, httpCode, &body});
          } else {
            // print out the error message
            Serial.printf("[HTTP] %s... failed, error: %s\n", method,
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
  void readMethod(Request::Method method, char *methodValue) {
    switch (method) {
    case Request::OPTIONS:
      strcpy(methodValue, "OPTIONS");
      break;

    case Request::DELETE:
      strcpy(methodValue, "DELETE");
      break;

    case Request::PATCH:
      strcpy(methodValue, "PATCH");
      break;

    case Request::PUT:
      strcpy(methodValue, "PUT");
      break;

    case Request::POST:
      strcpy(methodValue, "POST");
      break;

    case Request::HEAD:
      strcpy(methodValue, "HEAD");
      break;

    default:
      strcpy(methodValue, "GET");
      break;
    }
  }

  WiFiManager *wifiManager;
  Timer *timer;
  CertStore *certStore;
};
