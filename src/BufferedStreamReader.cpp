#include "BufferedStreamReader.h"

BufferedStreamReader::BufferedStreamReader(Stream* stream, bool isChunked)
    : _stream(stream),
      _strBuf(nullptr),
      _strLen(0),
      _pos(0),
      _len(0),
      _isChunked(isChunked),
      _chunkRemaining(0),
      _eof(false) {}

BufferedStreamReader::BufferedStreamReader(const char* str)
    : _stream(nullptr),
      _strBuf(str),
      _strLen(str ? strlen(str) : 0),
      _pos(0),
      _len(0),
      _isChunked(false),
      _chunkRemaining(0),
      _eof(false) {}

bool BufferedStreamReader::available() {
  if (_strBuf) {
    return _pos < _strLen;
  }
  if (_pos < _len) return true;
  if (_eof) return false;
  return refill();
}

int BufferedStreamReader::read() {
  if (!available()) return -1;
  if (_strBuf) {
    return (uint8_t)_strBuf[_pos++];
  }
  return (uint8_t)_buf[_pos++];
}

int BufferedStreamReader::peek() {
  if (!available()) return -1;
  if (_strBuf) {
    return (uint8_t)_strBuf[_pos];
  }
  return (uint8_t)_buf[_pos];
}

int BufferedStreamReader::readRawByte() {
  unsigned long start = millis();
  while (!_stream->available()) {
    if (millis() - start >= TIMEOUT_MS) return -1;
    yield();
  }
  return _stream->read();
}

bool BufferedStreamReader::refill() {
  if (_eof) return false;
  _pos = 0;
  _len = 0;

  if (!_isChunked) {
    unsigned long start = millis();
    while (!_stream->available()) {
      if (millis() - start >= TIMEOUT_MS) { _eof = true; return false; }
      yield();
    }
    size_t avail = _stream->available();
    if (avail > 0) {
      size_t toRead = (avail < BUF_SIZE) ? avail : BUF_SIZE;
      _len = _stream->readBytes(_buf, toRead);
    }
    if (_len == 0) _eof = true;
    return _len > 0;
  }

  while (_len < BUF_SIZE) {
    if (_chunkRemaining == 0) {
      char hexBuf[16];
      size_t hexIdx = 0;
      
      while (true) {
        int c = readRawByte();
        if (c == -1) { _eof = true; return _len > 0; }
        if (c == '\n') break;
        if (c != '\r' && hexIdx < sizeof(hexBuf) - 1) {
          hexBuf[hexIdx++] = (char)c;
        }
      }
      hexBuf[hexIdx] = '\0';
      
      char* semi = strchr(hexBuf, ';');
      if (semi) *semi = '\0';
      
      if (hexIdx == 0) continue; 

      _chunkRemaining = strtoul(hexBuf, nullptr, 16);
      
      if (_chunkRemaining == 0) {
        readRawByte();
        readRawByte();
        _eof = true;
        return _len > 0;
      }
    }

    size_t toRead = _chunkRemaining;
    if (toRead > (BUF_SIZE - _len)) toRead = BUF_SIZE - _len;
    
    unsigned long start = millis();
    while (!_stream->available()) {
       if (millis() - start >= TIMEOUT_MS) { _eof = true; return _len > 0; }
       yield();
    }
    
    size_t avail = _stream->available();
    if (avail < toRead) toRead = avail;
    
    size_t bytesRead = _stream->readBytes(_buf + _len, toRead);
    if (bytesRead == 0) { _eof = true; return _len > 0; }
    
    _len += bytesRead;
    _chunkRemaining -= bytesRead;

    if (_chunkRemaining == 0) {
      readRawByte();
      readRawByte();
    }
  }

  return _len > 0;
}
