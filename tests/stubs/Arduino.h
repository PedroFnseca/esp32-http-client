#ifndef ARDUINO_H
#define ARDUINO_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

inline unsigned long millis() { 
  static unsigned long time_ms = 0;
  time_ms += 100;
  return time_ms;
}
inline void delay(unsigned long) {}

class String {
 public:
  String() = default;
  String(const char* value) : _data(value ? value : "") {}
  String(const std::string& value) : _data(value) {}
  String(int value) : _data(std::to_string(value)) {}
  String(long value) : _data(std::to_string(value)) {}
  String(unsigned long value) : _data(std::to_string(value)) {}
  String(float value) : _data(std::to_string(value)) {}
  String(double value) : _data(std::to_string(value)) {}
  String(char c) : _data(1, c) {}

  String& operator=(char c) {
    _data = std::string(1, c);
    return *this;
  }

  int indexOf(const char* needle, int fromIndex = 0) const {
    if (!needle) return -1;
    if (fromIndex < 0) fromIndex = 0;
    size_t pos = _data.find(needle, static_cast<size_t>(fromIndex));
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }

  int indexOf(char c, int fromIndex = 0) const {
    if (fromIndex < 0) fromIndex = 0;
    size_t pos = _data.find(c, static_cast<size_t>(fromIndex));
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }

  bool startsWith(const char* prefix) const {
    if (!prefix) return false;
    size_t len = strlen(prefix);
    if (len > _data.size()) return false;
    return _data.compare(0, len, prefix) == 0;
  }

  bool startsWith(const String& prefix) const {
    return startsWith(prefix.c_str());
  }

  bool endsWith(const char* suffix) const {
    if (!suffix) return false;
    size_t len = strlen(suffix);
    if (len > _data.size()) return false;
    return _data.compare(_data.size() - len, len, suffix) == 0;
  }

  bool endsWith(const String& suffix) const {
    return endsWith(suffix.c_str());
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

  bool replace(const String& find, const String& replaceStr) {
    return replace(find.c_str(), replaceStr.c_str());
  }

  bool replace(const char* find, const char* replaceStr) {
    if (!find || !replaceStr) return false;
    size_t findLen = strlen(find);
    if (findLen == 0) return false;
    size_t pos = 0;
    bool replaced = false;
    while ((pos = _data.find(find, pos)) != std::string::npos) {
      _data.replace(pos, findLen, replaceStr);
      pos += strlen(replaceStr);
      replaced = true;
    }
    return replaced;
  }

  void reserve(size_t size) {
    _data.reserve(size);
  }

  String& operator+=(const String& other) {
    _data += other._data;
    return *this;
  }

  String& operator+=(const char* other) {
    _data += (other ? other : "");
    return *this;
  }

  String& operator+=(char c) {
    _data += c;
    return *this;
  }

  size_t length() const {
    return _data.length();
  }

  bool isEmpty() const {
    return _data.empty();
  }

  void clear() {
    _data.clear();
  }

  bool operator==(const String& other) const {
    return _data == other._data;
  }

  bool operator==(const char* other) const {
    return _data == (other ? other : "");
  }

  bool operator!=(const String& other) const {
    return _data != other._data;
  }

  bool operator!=(const char* other) const {
    return _data != (other ? other : "");
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

inline void yield() {}

class Stream {
 public:
  virtual ~Stream() = default;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  
  virtual size_t readBytes(char* buffer, size_t length) {
    size_t count = 0;
    while (count < length && available()) {
      int c = read();
      if (c < 0) break;
      buffer[count++] = static_cast<char>(c);
    }
    return count;
  }
};

#endif
