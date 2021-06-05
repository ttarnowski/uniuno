#pragma once

namespace uniuno {

class ErrorBase {
public:
  virtual ~ErrorBase(){};
  virtual operator const char *() = 0;
};

} // namespace uniuno