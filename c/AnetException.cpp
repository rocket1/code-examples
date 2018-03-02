#include "AnetException.h"

AnetException::AnetException(const std::string& msg) : std::runtime_error(msg) {}
AnetExceptionBrokenLink::AnetExceptionBrokenLink(const std::string& msg) : AnetException(msg) {}
AnetExceptionConnFail::AnetExceptionConnFail(const std::string& msg) : AnetException(msg) {}

