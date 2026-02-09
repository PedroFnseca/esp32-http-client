#ifndef REST_TYPES_H
#define REST_TYPES_H

#include <Arduino.h>

enum HttpMethod { HTTP_GET_METHOD, HTTP_POST_METHOD, HTTP_PUT_METHOD, HTTP_DELETE_METHOD, HTTP_PATCH_METHOD };

enum DataType { TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_STRING, TYPE_DOUBLE, TYPE_LONG };

struct KeyValue {
  String key;
  String value;
  bool quoteValue;
};

struct ResponseBinding {
  const char* key;
  void* target;
  DataType type;
  size_t size;
};

struct HttpHeader {
  String name;
  String value;
};

#endif
