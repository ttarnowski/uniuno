#pragma once

class ErrorBase {
public:
  virtual ~ErrorBase(){};
  virtual operator const char *() = 0;
};
