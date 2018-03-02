#ifndef _ANET_EXCEPTION_H_
#define _ANET_EXCEPTION_H_

#include <stdexcept>

class AnetException : public std::runtime_error {

 public:
  AnetException(const std::string&);
};

class AnetExceptionBrokenLink : public AnetException {

 public:
  AnetExceptionBrokenLink(const std::string&);
};

class AnetExceptionConnFail : public AnetException {

 public:
  AnetExceptionConnFail(const std::string&);
};

#endif
