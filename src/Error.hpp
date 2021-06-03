#pragma once

#include <ErrorBase.hpp>

class Error : public ErrorBase {
public:
  Error() { this->err = "error"; }

  Error(ErrorBase *err) { this->err = (const char *)err; }

  Error(const char *err) { this->err = err; }

  operator const char *() override { return this->err; }

private:
  const char *err;
};