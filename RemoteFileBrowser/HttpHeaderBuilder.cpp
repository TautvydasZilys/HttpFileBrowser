#include "PrecompiledHeader.h"
#include "HttpHeaderBuilder.h"

using namespace std;

string HttpHeaderBuilder::BuildPostHeader(const std::string& path, const std::string& contentType, int contentLength)
{
	stringstream postHeader;

	postHeader << "POST " << path << endl;
	postHeader << "Content-Type: " << contentType << endl;
	postHeader << "Content-Length: " << contentLength << endl << endl;

	return postHeader.str();
}