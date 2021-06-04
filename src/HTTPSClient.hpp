#pragma once

#include <CertStoreBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <WiFiConnector.hpp>
#include <logging.h>
#include <memory>
#include <set_clock.hpp>

class BodyStream : public Stream {
public:
  BodyStream() : wifiClient(), httpClient() {}
  ~BodyStream() { this->httpClient.end(); }

  int available() {
    this->checkBytesLeft();

    if (!this->httpClient.connected()) {
      return 0;
    }

    return this->bytesLeft;
  }

  size_t readBytes(uint8_t *buffer, size_t length) {
    this->checkBytesLeft();

    if (this->bytesLeft == 0) {
      return 0;
    }

    int bytesRead = this->wifiClient.readBytes(
        buffer, std::min((size_t)this->bytesLeft, length));

    if (this->bytesLeft > 0) {
      this->bytesLeft -= bytesRead;
    }

    return bytesRead;
  }

  size_t write(uint8_t buffer) {
    return this->wifiClient.write(&buffer, sizeof(buffer));
  }
  int read() { return this->wifiClient.read(); }
  int peek() { return this->wifiClient.peek(); }

  String readString() { return this->wifiClient.readString(); }

  void checkBytesLeft() {
    if (this->bytesLeft == -2) {
      this->bytesLeft = this->httpClient.getSize();
    }
  }

  BearSSL::WiFiClientSecure &getWiFiClient() { return this->wifiClient; }
  HTTPClient &getHTTPClient() { return this->httpClient; }

  void setWiFiClientCertStore(BearSSL::CertStore *certStore) {
    this->wifiClient.setCertStore(certStore);
  }

private:
  BearSSL::WiFiClientSecure wifiClient;
  HTTPClient httpClient;
  int bytesLeft = -2;
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
  std::shared_ptr<BodyStream> body;
};

class HTTPSClient {
public:
  HTTPSClient(CertStore *cert_store, WiFiConnector *wifi_connector) {
    this->cert_store = cert_store;
    this->wifi_connector = wifi_connector;
  }

  Future<void, Response> send_request(Request request) {
    return set_clock(this->wifi_connector).and_then(create_future([=](time_t) {
      return this->try_to_send_request(request);
    }));
  }

private:
  AsyncResult<Response> try_to_send_request(Request request) {
    auto body = std::make_shared<BodyStream>();

    body->setWiFiClientCertStore(this->cert_store);

    HTTPClient &http = body->getHTTPClient();

    DEBUG("[HTTP] begin...\n");

    if (http.begin(body->getWiFiClient(), request.url)) {
      char method[10];
      this->readMethod(request.method, method);

      DEBUGF("[HTTP] %s %s\n", method, request.url);
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
        DEBUGF("[HTTP] %s... code: %d\n", method, httpCode);

        return AsyncResult<Response>::resolve(
            Response{nullptr, httpCode, body});
      } else {
        // print out the error message
        ERRORF("[HTTP] %s... failed, error: %s\n", method,
               http.errorToString(httpCode).c_str());
        return AsyncResult<Response>::reject(
            Error(http.errorToString(httpCode).c_str()));
      }
    } else {
      ERROR("[HTTP] Unable to connect\n");
      return AsyncResult<Response>::reject(Error("unable to connect"));
    }
  }

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

  WiFiConnector *wifi_connector;
  CertStore *cert_store;
};
