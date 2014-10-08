#include "PrecompiledHeader.h"
#include "HttpRequest.h"
#include "RestCommunicator.h"

using namespace std;

static void SendPostRequest(SOCKET s, const Http::Request& httpRequest)
{
	auto header = httpRequest.BuildHeaderString();

	auto result = send(s, header.c_str(), header.length(), 0);
	if (result != header.length()) goto fail;

	result = send(s, httpRequest.content.c_str(), httpRequest.content.length(), 0);
	if (result != httpRequest.content.length()) goto fail;

	return;

fail:
	Assert(false);	
	Utilities::Logging::LogErrorIfFailed(true, L"Failed to send post request: ");
}

void RestCommunicator::Post(SOCKET s, string&& hostname, string&& path, const string& key, const string& value)
{
	Http::Request request;

	request.requestVerb = Http::RequestVerb::POST;
	request.contentType = Http::ContentType::JSON;
	request.hostname = std::move(hostname);
	request.requestPath = std::move(path);

	// Body format:
	// {"<KEY>":"<VALUE>"}
	auto length = 0;
	request.content.resize(7 + key.length() + value.length());

	request.content[length++] = '{';
	request.content[length++] = '\"';

	memcpy(&request.content[length], key.data(), key.length());
	length += key.length();

	request.content[length++] = '\"';
	request.content[length++] = ':';
	request.content[length++] = '\"';

	memcpy(&request.content[length], value.data(), value.length());
	length += value.length();

	request.content[length++] = '\"';
	request.content[length++] = '}';
	
	SendPostRequest(s, request);
}

bool RestCommunicator::ReceiveResponse(SOCKET s)
{
	const int kBufferLength = 1024;
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

bool RestCommunicator::ReceivePost(SOCKET s, unordered_map<string, string>& results)
{
	results.clear();

	const Http::Request receivedRequest(s);

	if (receivedRequest.requestVerb != Http::RequestVerb::POST || receivedRequest.contentType != Http::ContentType::JSON)
	{
		return false;
	}

	const auto& content = receivedRequest.content;

	// Make sure it really has a JSON body
	if (content.length() < 2 || content[0] != '{' ||
		content[content.length() - 1] != '}')
	{
		return false;
	}

	auto position = 1;
	while (position < content.length() - 1)
	{
		auto parenthesis = content.find('\"', position);
		if (parenthesis == string::npos) return false;

		auto closingParenthesis = content.find('\"', parenthesis + 1);
		if (closingParenthesis == string::npos) return false;

		auto key = content.substr(parenthesis + 1, closingParenthesis - parenthesis - 1);

		position = closingParenthesis + 1;
		if (position + 1 >= content.length() || content[position] != ':') return false;

		parenthesis = content.find('\"', position + 2);
		if (parenthesis == string::npos) return false;

		closingParenthesis = content.find('\"', parenthesis + 1);
		if (closingParenthesis == string::npos) return false;

		auto value = content.substr(parenthesis + 1, closingParenthesis - parenthesis - 1);

		results.emplace(std::move(key), std::move(value));
		position = closingParenthesis + 1;
	}

	return true;
}