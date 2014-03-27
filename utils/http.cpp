#include <ext/happyhttp.h>
#include <utils/http.h>

using namespace std;

void Utils::http_request(const char* remoteAddr, unsigned int remotePort, const char** headers, const char* route, const char* method, const unsigned char* query, unsigned int size, function<void(signed int, string&)> result)
{
	static signed int code;
	string result_data;

	{
		happyhttp::Connection conn(remoteAddr, remotePort);

		conn.setcallbacks(
		[](const happyhttp::Response* resp, void*)
		{
			code = resp->getstatus();
		},
		[](const happyhttp::Response*, void* userdata, const unsigned char* data, int n)
		{
			string& result_data = *reinterpret_cast<string*>(userdata);
			result_data.append(reinterpret_cast<const char*>(data), n);
		},
		[](const happyhttp::Response*, void*)
		{
		}, reinterpret_cast<void*>(&result_data));

		conn.request(method, route, headers, query, size);

		while (conn.outstanding())
			conn.pump();
	}

	result(code, result_data);
}
