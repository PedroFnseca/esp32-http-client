#ifndef HTTP_ENCODING_H
#define HTTP_ENCODING_H

#include <string>

namespace http_encoding {

std::string urlEncode(const std::string& input);
std::string escapeJson(const std::string& input);
std::string buildUrl(const std::string& baseUrl, int port, const std::string& path);

}  // namespace http_encoding

#endif
