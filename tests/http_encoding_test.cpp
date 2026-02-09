#include <cassert>
#include <iostream>

#include "../src/HttpEncoding.h"

int main() {
  using http_encoding::buildUrl;
  using http_encoding::escapeJson;
  using http_encoding::urlEncode;

  assert(urlEncode("simple") == "simple");
  assert(urlEncode("São Paulo") == "S%C3%A3o%20Paulo");
  assert(urlEncode("a+b&c=d") == "a%2Bb%26c%3Dd");

  assert(escapeJson("plain") == "plain");
  assert(escapeJson("a\"b") == "a\\\"b");
  assert(escapeJson("line\nnext") == "line\\nnext");

  assert(buildUrl("https://api.example.com", 0, "/v1") == "https://api.example.com/v1");
  assert(buildUrl("https://api.example.com/base", 8443, "items") == "https://api.example.com:8443/base/items");
  assert(buildUrl("http://localhost", 8080, "") == "http://localhost:8080/");

  std::cout << "All unit tests passed.\n";
  return 0;
}
