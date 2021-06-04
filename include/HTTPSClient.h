#pragma once

#include <CertStoreBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <WiFiConnector.h>
#include <logging.h>
#include <memory>
#include <set_clock.h>

class ResponseStream : public Stream {
public:
  ResponseStream() : wifi_client(), http_client() {}
  ~ResponseStream() {
    if (this->x509_list != nullptr) {
      free(this->x509_list);
    }
    this->http_client.end();
  }

  int available() {
    this->check_bytes_left();

    if (!this->http_client.connected()) {
      return 0;
    }

    return this->bytes_left;
  }

  size_t readBytes(uint8_t *buffer, size_t length) {
    this->check_bytes_left();

    if (this->bytes_left == 0) {
      return 0;
    }

    int bytesRead = this->wifi_client.readBytes(
        buffer, std::min((size_t)this->bytes_left, length));

    if (this->bytes_left > 0) {
      this->bytes_left -= bytesRead;
    }

    return bytesRead;
  }

  size_t write(uint8_t buffer) {
    return this->wifi_client.write(&buffer, sizeof(buffer));
  }
  int read() { return this->wifi_client.read(); }
  int peek() { return this->wifi_client.peek(); }

  String readString() { return this->wifi_client.readString(); }

  void check_bytes_left() {
    if (this->bytes_left == -2) {
      this->bytes_left = this->http_client.getSize();
    }
  }

  BearSSL::WiFiClientSecure &get_wifi_client() { return this->wifi_client; }
  HTTPClient &get_HTTP_client() { return this->http_client; }

  void set_wifi_client_cert_store(BearSSL::CertStore *cert_store) {
    this->wifi_client.setCertStore(cert_store);
  }

  void set_wifi_client_trust_anchors(const char *pem_cert) {
    this->x509_list = new BearSSL::X509List(pem_cert);
    this->wifi_client.setTrustAnchors(this->x509_list);
  }

private:
  BearSSL::WiFiClientSecure wifi_client;
  HTTPClient http_client;
  BearSSL::X509List *x509_list;
  int bytes_left = -2;
};

class RequestBuilder;

class Request {
public:
  enum Method { GET, HEAD, POST, PUT, PATCH, DELETE, OPTIONS };

  static RequestBuilder build(Request::Method method, const char *url);
  const char *url;
  Request::Method method;
  Stream *body = nullptr;
  size_t body_size = 0;
  const char *body_cstr = nullptr;
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
    request.body_cstr = b;
    request.body_size = strlen(b);
    request.body = nullptr;
    return *this;
  }

  RequestBuilder &body(Stream *s, size_t size) {
    request.body = s;
    request.body_size = size;
    request.body_cstr = nullptr;

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
  int status_code;
  std::shared_ptr<ResponseStream> body;
};

class HTTPSClient {
public:
  HTTPSClient(CertStore *cert_store, WiFiConnector *wifi_connector) {
    this->cert_store = cert_store;
    this->wifi_connector = wifi_connector;
  }

  HTTPSClient(WiFiConnector *wifi_connector, const char *host,
              const char *pem_cert) {
    this->wifi_connector = wifi_connector;
    this->host = host;
    this->pem_cert = pem_cert;
  }

  Future<void, Response> send_request(Request request) {
    return set_clock(this->wifi_connector).and_then(create_future([=](time_t) {
      return this->try_to_send_request(request);
    }));
  }

private:
  AsyncResult<Response> try_to_send_request(Request request) {
    auto body = std::make_shared<ResponseStream>();

    if (host != nullptr) {
      DEBUG("setting single pem certificate on ssl client");
      body->set_wifi_client_trust_anchors(this->pem_cert);

      if (strstr(request.url, this->host) == nullptr) {
        return AsyncResult<Response>::reject(Error("invalid host"));
      }
    } else {
      DEBUG("setting certificate store on ssl client");
      body->set_wifi_client_cert_store(this->cert_store);
    }

    HTTPClient &http = body->get_HTTP_client();

    DEBUG("[HTTP] begin...\n");

    if (http.begin(body->get_wifi_client(), request.url)) {
      char method[10];
      this->readMethod(request.method, method);

      DEBUGF("[HTTP] %s %s\n", method, request.url);
      // start connection and send HTTP header, set the HTTP method and
      // request body
      for (auto &h : request.headers) {
        http.addHeader(h.first, h.second);
      }

      int http_status_code;

      if (request.body == nullptr && request.body_cstr == nullptr) {
        http_status_code = http.sendRequest(method, (uint8_t *)nullptr, 0);
      } else if (request.body != nullptr) {
        http_status_code =
            http.sendRequest(method, request.body, request.body_size);
      } else {
        http_status_code = http.sendRequest(method, String(request.body_cstr));
      }

      // http_status_code will be negative on error
      if (http_status_code > 0) {
        // HTTP header has been send and Server response header has been
        // handled
        DEBUGF("[HTTP] %s... code: %d\n", method, http_status_code);

        return AsyncResult<Response>::resolve(
            Response{nullptr, http_status_code, body});
      } else {
        // print out the error message
        ERRORF("[HTTP] %s... failed, error: %s\n", method,
               http.errorToString(http_status_code).c_str());
        return AsyncResult<Response>::reject(
            Error(http.errorToString(http_status_code).c_str()));
      }
    } else {
      ERROR("[HTTP] Unable to connect\n");
      return AsyncResult<Response>::reject(Error("unable to connect"));
    }
  }

  void readMethod(Request::Method method, char *method_value) {
    switch (method) {
    case Request::OPTIONS:
      strcpy(method_value, "OPTIONS");
      break;

    case Request::DELETE:
      strcpy(method_value, "DELETE");
      break;

    case Request::PATCH:
      strcpy(method_value, "PATCH");
      break;

    case Request::PUT:
      strcpy(method_value, "PUT");
      break;

    case Request::POST:
      strcpy(method_value, "POST");
      break;

    case Request::HEAD:
      strcpy(method_value, "HEAD");
      break;

    default:
      strcpy(method_value, "GET");
      break;
    }
  }

  WiFiConnector *wifi_connector;
  CertStore *cert_store;

  const char *host;
  const char *pem_cert;
};
