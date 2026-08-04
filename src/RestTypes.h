#ifndef REST_TYPES_H
#define REST_TYPES_H

#include <Arduino.h>
#include <functional>
#include <stddef.h>
#include <type_traits>
#include <vector>

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

#define REST_FIELD(f) visitor.field(#f, f)
#define REST_FIELD_NAMED(k, f) visitor.field(k, f)

#define REST_JSON_MAP(...) \
  template <typename RestVisitor_T> \
  void rest_json_map(RestVisitor_T& visitor) { \
    __VA_ARGS__; \
  } \
  template <typename RestVisitor_T> \
  void rest_json_map(RestVisitor_T& visitor) const { \
    __VA_ARGS__; \
  }

#define REST_FIELD_EXT(f) visitor.field(#f, obj.f)
#define REST_FIELD_EXT_NAMED(k, f) visitor.field(k, obj.f)

#define REST_JSON_MAP_EXT(StructType, ...) \
  template <typename RestVisitor_T> \
  inline void rest_json_map_external(StructType& obj, RestVisitor_T& visitor) { \
    __VA_ARGS__; \
  } \
  template <typename RestVisitor_T> \
  inline void rest_json_map_external(const StructType& obj, RestVisitor_T& visitor) { \
    __VA_ARGS__; \
  }

struct RestDummyVisitor {
  template <typename V>
  void field(const char*, V&) {}
  template <typename V>
  void field(const char*, const V&) {}
};

template <typename T, typename = void>
struct HasRestJsonMapMember : std::false_type {};

template <typename T>
struct HasRestJsonMapMember<T, decltype(std::declval<T&>().rest_json_map(std::declval<RestDummyVisitor&>()), void())> : std::true_type {};

template <typename T, typename = void>
struct HasRestJsonMapExternal : std::false_type {};

template <typename T>
struct HasRestJsonMapExternal<T, decltype(rest_json_map_external(std::declval<T&>(), std::declval<RestDummyVisitor&>()), void())> : std::true_type {};

template <typename T>
struct HasRestJsonMap : std::integral_constant<bool, HasRestJsonMapMember<T>::value || HasRestJsonMapExternal<T>::value> {};

template <typename T, typename Visitor>
inline typename std::enable_if<HasRestJsonMapMember<T>::value, void>::type invokeRestJsonMap(T& obj, Visitor& visitor) {
  obj.rest_json_map(visitor);
}

template <typename T, typename Visitor>
inline typename std::enable_if<HasRestJsonMapMember<T>::value, void>::type invokeRestJsonMap(const T& obj, Visitor& visitor) {
  obj.rest_json_map(visitor);
}

template <typename T, typename Visitor>
inline typename std::enable_if<!HasRestJsonMapMember<T>::value && HasRestJsonMapExternal<T>::value, void>::type invokeRestJsonMap(T& obj, Visitor& visitor) {
  rest_json_map_external(obj, visitor);
}

template <typename T, typename Visitor>
inline typename std::enable_if<!HasRestJsonMapMember<T>::value && HasRestJsonMapExternal<T>::value, void>::type invokeRestJsonMap(const T& obj, Visitor& visitor) {
  rest_json_map_external(obj, visitor);
}

class RestJsonSerializer {
 public:
  String json;
  bool first;

  RestJsonSerializer() : first(true) {
    json.reserve(128);
    json += "{";
  }

  void appendKey(const char* key) {
    if (!first) json += ",";
    first = false;
    json += "\"";
    json += key;
    json += "\":";
  }

  void field(const char* key, int val) {
    appendKey(key);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    json += buf;
  }

  void field(const char* key, long val) {
    appendKey(key);
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", val);
    json += buf;
  }

  void field(const char* key, float val) {
    appendKey(key);
    char buf[32];
    snprintf(buf, sizeof(buf), "%.5g", val);
    json += buf;
  }

  void field(const char* key, double val) {
    appendKey(key);
    char buf[32];
    snprintf(buf, sizeof(buf), "%.9g", val);
    json += buf;
  }

  void field(const char* key, bool val) {
    appendKey(key);
    json += val ? "true" : "false";
  }

  void field(const char* key, const char* val) {
    appendKey(key);
    if (!val) {
      json += "null";
    } else {
      json += "\"";
      json += val;
      json += "\"";
    }
  }

  void field(const char* key, char* val) {
    field(key, (const char*)val);
  }

  template <size_t N>
  void field(const char* key, const char (&val)[N]) {
    field(key, (const char*)val);
  }

  template <size_t N>
  void field(const char* key, char (&val)[N]) {
    field(key, (const char*)val);
  }

  void field(const char* key, const String& val) {
    appendKey(key);
    json += "\"";
    json += val;
    json += "\"";
  }

  String finish() {
    json += "}";
    return json;
  }
};

class RestJsonBinder {
 public:
  std::vector<ResponseBinding>& bindings;
  std::vector<String>& storage;
  const char* basePath;

  RestJsonBinder(std::vector<ResponseBinding>& b, std::vector<String>& s, const char* base = "")
      : bindings(b), storage(s), basePath(base) {
    if (basePath && basePath[0] != '\0') {
      storage.reserve(storage.size() + 128);
    }
  }

  const char* makeKey(const char* key) {
    if (!basePath || basePath[0] == '\0') {
      return key;
    }
    storage.push_back(String(basePath) + "." + key);
    return storage.back().c_str();
  }

  void field(const char* key, int& val) {
    bindings.push_back({makeKey(key), &val, TYPE_INT, sizeof(int)});
  }

  void field(const char* key, long& val) {
    bindings.push_back({makeKey(key), &val, TYPE_LONG, sizeof(long)});
  }

  void field(const char* key, float& val) {
    bindings.push_back({makeKey(key), &val, TYPE_FLOAT, sizeof(float)});
  }

  void field(const char* key, double& val) {
    bindings.push_back({makeKey(key), &val, TYPE_DOUBLE, sizeof(double)});
  }

  void field(const char* key, bool& val) {
    bindings.push_back({makeKey(key), &val, TYPE_BOOL, sizeof(bool)});
  }

  template <size_t N>
  void field(const char* key, char (&val)[N]) {
    bindings.push_back({makeKey(key), val, TYPE_STRING, N});
  }

  void field(const char* key, char* val, size_t maxLen = 64) {
    bindings.push_back({makeKey(key), val, TYPE_STRING, maxLen});
  }

  void field(const char* key, String& val) {
    bindings.push_back({makeKey(key), &val, TYPE_ARDUINO_STRING, 0});
  }
};

#endif
