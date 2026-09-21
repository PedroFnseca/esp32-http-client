#ifndef SOAP_TYPES_H
#define SOAP_TYPES_H

#include <Arduino.h>
#include <functional>
#include <vector>

#include "RestTypes.h"

enum SoapVersion {
  SOAP_1_1,
  SOAP_1_2
};

struct SoapFault {
  bool matched = false;
  String faultCode;
  String faultString;
  String faultActor;
  String detail;
};

typedef std::function<void(const SoapFault&)> SoapFaultCallback;

#endif
