#ifndef BUFFERED_STREAM_READER_H
#define BUFFERED_STREAM_READER_H

#include <Arduino.h>

class BufferedStreamReader {
 public:
  static constexpr size_t   BUF_SIZE   = 512;
  static constexpr uint32_t TIMEOUT_MS = 2000;

  explicit BufferedStreamReader(Stream* stream, bool isChunked = false);
  explicit BufferedStreamReader(const char* str);

  bool available();
  int  read();
  int  peek();

 private:
  Stream*     _stream;
  const char* _strBuf;
  size_t      _strLen;
  char        _buf[BUF_SIZE];
  size_t      _pos;
  size_t      _len;

  bool   _isChunked;
  size_t _chunkRemaining;
  bool   _eof;

  int  readRawByte();
  bool refill();
};

#endif
