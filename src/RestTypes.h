#ifndef REST_TYPES_H
#define REST_TYPES_H

#include <functional>
#include <stddef.h>

enum HttpMethod { HTTP_GET_METHOD, HTTP_POST_METHOD, HTTP_PUT_METHOD, HTTP_DELETE_METHOD, HTTP_PATCH_METHOD };

enum DataType { TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_STRING, TYPE_DOUBLE, TYPE_LONG, TYPE_ARDUINO_STRING };

typedef std::function<void(int)> HttpResponseCallback;
typedef std::function<void(int, const char*)> HttpErrorCallback;

struct KeyValue {
  const char* key;
  const char* value;
  char valueBuffer[64];
  bool quoteValue;
};

struct ResponseBinding {
  const char* key;
  void* target;
  DataType type;
  size_t size;
};

struct HttpHeader {
  char name[64];
  char value[256];
};

#endif
