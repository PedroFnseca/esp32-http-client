#ifndef ARDUINO_H
#define ARDUINO_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

class String {
 public:
  String() = default;
  String(const char* value) : _data(value ? value : "") {
  }
  String(const std::string& value) : _data(value) {
  }
  String(int value) : _data(std::to_string(value)) {
  }

  int indexOf(const char* needle, int fromIndex = 0) const {
    if (!needle) return -1;
    if (fromIndex < 0) fromIndex = 0;
    size_t pos = _data.find(needle, static_cast<size_t>(fromIndex));
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }

  String substring(int beginIndex, int endIndex) const {
    if (beginIndex < 0) beginIndex = 0;
    if (endIndex < beginIndex) endIndex = beginIndex;
    if (beginIndex > static_cast<int>(_data.size())) beginIndex = static_cast<int>(_data.size());
    if (endIndex > static_cast<int>(_data.size())) endIndex = static_cast<int>(_data.size());
    return String(_data.substr(static_cast<size_t>(beginIndex), static_cast<size_t>(endIndex - beginIndex)));
  }

  String substring(int beginIndex) const {
    if (beginIndex < 0) beginIndex = 0;
    if (beginIndex > static_cast<int>(_data.size())) beginIndex = static_cast<int>(_data.size());
    return String(_data.substr(static_cast<size_t>(beginIndex)));
  }

  String& operator+=(const String& other) {
    _data += other._data;
    return *this;
  }

  String& operator+=(const char* other) {
    _data += (other ? other : "");
    return *this;
  }

  const char* c_str() const {
    return _data.c_str();
  }

  std::string str() const {
    return _data;
  }

 private:
  std::string _data;
};

inline String operator+(const String& left, const String& right) {
  return String(left.str() + right.str());
}

inline String operator+(const String& left, const char* right) {
  return String(left.str() + std::string(right ? right : ""));
}

inline String operator+(const char* left, const String& right) {
  return String(std::string(left ? left : "") + right.str());
}

class Stream {
 public:
  virtual ~Stream() = default;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
};

#endif
