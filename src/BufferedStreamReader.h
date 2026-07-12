#ifndef BUFFERED_STREAM_READER_H
#define BUFFERED_STREAM_READER_H

#include <Arduino.h>

class BufferedStreamReader {
 public:
  static constexpr size_t   BUF_SIZE   = 512;
  static constexpr uint32_t TIMEOUT_MS = 2000;

  explicit BufferedStreamReader(Stream* stream)
      : _stream(stream), _pos(0), _len(0) {}

  bool available() {
    if (_pos < _len) return true;
    return refill();
  }

  int read() {
    if (_pos >= _len && !refill()) return -1;
    return (uint8_t)_buf[_pos++];
  }

  int peek() {
    if (_pos >= _len && !refill()) return -1;
    return (uint8_t)_buf[_pos];
  }

 private:
  Stream* _stream;
  char    _buf[BUF_SIZE];
  size_t  _pos;  // next byte to serve from _buf
  size_t  _len;  // valid bytes currently in _buf

  bool refill() {
    _pos = 0;
    _len = 0;

    unsigned long start = millis();
    while (!_stream->available()) {
      if (millis() - start >= TIMEOUT_MS) return false;
      delay(1);
    }

    while (_stream->available() && _len < BUF_SIZE) {
      _buf[_len++] = (char)_stream->read();
    }

    return _len > 0;
  }
};

#endif
