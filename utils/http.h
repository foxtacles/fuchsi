#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <functional>

namespace Utils
{
	void http_request(const char* remoteAddr, unsigned int remotePort, const char** headers, const char* route, const char* method, const unsigned char* query, unsigned int size, std::function<void(signed int, const std::string&)> result);
}

#endif
