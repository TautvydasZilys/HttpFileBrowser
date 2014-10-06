#include "PrecompiledHeader.h"
#include "HttpHeaderBuilder.h"

using namespace std;

string HttpHeaderBuilder::BuildPostHeader(const std::string& hostname, const std::string& path, const std::string& contentType, int contentLength)
{
	stringstream postHeader;

	postHeader << "POST " << path << " HTTP/1.1" << endl;
	postHeader << "Host: " << hostname << endl;
	postHeader << "Connection: Keep-Alive" << endl;
	postHeader << "Content-Type: " << contentType << endl;
	postHeader << "Content-Length: " << contentLength << endl << endl;

	return postHeader.str();
}