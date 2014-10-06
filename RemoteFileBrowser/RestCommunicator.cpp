#include "PrecompiledHeader.h"
#include "HttpHeaderBuilder.h"
#include "RestCommunicator.h"

using namespace std;

static void SendPostRequest(SOCKET s, const string& hostname, const string& path, const string& httpBody)
{
	auto header = HttpHeaderBuilder::BuildPostHeader(hostname, path, "application/json", httpBody.length());

	auto result = send(s, header.c_str(), header.length(), 0);
	if (result != header.length()) goto fail;

	result = send(s, httpBody.c_str(), httpBody.length(), 0);
	if (result != httpBody.length()) goto fail;

	return;

fail:
	Assert(false);	
	Utilities::Logging::LogErrorIfFailed(true, L"Failed to send post request: ");
}

void RestCommunicator::Post(SOCKET s, const string& hostname, const string& path, const string& key, const string& value)
{
	// Body format:
	// {"<KEY>":"<VALUE>"}
	string httpBody;
	auto length = 0;
	httpBody.resize(7 + key.length() + value.length());

	httpBody[length++] = '{';
	httpBody[length++] = '\"';

	memcpy(&httpBody[length], key.data(), key.length());
	length += key.length();

	httpBody[length++] = '\"';
	httpBody[length++] = ':';
	httpBody[length++] = '\"';

	memcpy(&httpBody[length], value.data(), value.length());
	length += value.length();

	httpBody[length++] = '\"';
	httpBody[length++] = '}';

	SendPostRequest(s, hostname, path, httpBody);
}

bool RestCommunicator::ReceiveResponse(SOCKET s)
{
	const int kBufferLength = 256;
	char buffer[kBufferLength];

	auto bytesReceived = recv(s, buffer, kBufferLength, 0);

	if (bytesReceived < 1)
	{
		return false;
	}

	auto bufferEnd = buffer + kBufferLength;
	auto newLine = std::find(buffer, bufferEnd, '\n');

	if (newLine == bufferEnd)
	{
		return false;
	}

	*newLine = '\0';
	return strcmp(buffer, "HTTP/1.1 200 OK") == 0;
}

bool RestCommunicator::Receive(SOCKET s, unordered_map<string, string>& results)
{
	// TODO: implement

	results.clear();

	const int kBufferLength = 2560;
	char buffer[kBufferLength];
	auto bytesReceived = recv(s, buffer, kBufferLength, 0);

	if (bytesReceived < 1)
	{
		return false;
	}

	return true;
}