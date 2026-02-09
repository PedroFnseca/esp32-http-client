#include "HttpEncoding.h"

#include <cctype>

namespace http_encoding {

std::string urlEncode(const std::string& input) {
  static const char* kHex = "0123456789ABCDEF";
  std::string encoded;
  encoded.reserve(input.size() * 3);

  for (unsigned char c : input) {
    const bool safe = std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~';

    if (safe) {
      encoded.push_back(static_cast<char>(c));
      continue;
    }

    encoded.push_back('%');
    encoded.push_back(kHex[(c >> 4) & 0x0F]);
    encoded.push_back(kHex[c & 0x0F]);
  }

  return encoded;
}

std::string escapeJson(const std::string& input) {
  std::string out;
  out.reserve(input.size() + 8);

  for (char c : input) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
    }
  }

  return out;
}

std::string buildUrl(const std::string& baseUrl, int port, const std::string& path) {
  std::string urlBase = baseUrl;

  if (port != 0) {
    const std::size_t protoEnd = urlBase.find("://");
    const std::size_t startSearch = (protoEnd != std::string::npos) ? protoEnd + 3 : 0;
    const std::size_t slashPos = urlBase.find('/', startSearch);

    if (slashPos != std::string::npos) {
      urlBase.insert(slashPos, ":" + std::to_string(port));
    } else {
      urlBase += ":" + std::to_string(port);
    }
  }

  std::string normalizedPath = path.empty() ? "/" : path;
  if (normalizedPath.front() != '/') normalizedPath = "/" + normalizedPath;

  return urlBase + normalizedPath;
}

}  // namespace http_encoding
